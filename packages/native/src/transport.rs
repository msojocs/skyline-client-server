use std::collections::{HashMap, VecDeque};
use std::io::{self, Read, Write};
use std::net::{Shutdown, TcpListener, TcpStream, ToSocketAddrs};
use std::sync::{Arc, Condvar, Mutex};
use std::thread::{self, JoinHandle};
use std::time::{Duration, Instant};

pub const HANDSHAKE: u32 = 114514;
pub const MAX_MESSAGE_BYTES: usize = 64 * 1024 * 1024;
const IO_TIMEOUT: Duration = Duration::from_secs(5);

#[derive(Debug)]
pub struct Message {
    pub id: u64,
    pub body: String,
    pub generation: u64,
}

pub fn read_frame(reader: &mut impl Read) -> io::Result<(u64, String)> {
    let mut header = [0; 12];
    reader.read_exact(&mut header)?;
    let len = u32::from_be_bytes(header[..4].try_into().unwrap()) as usize;
    if len > MAX_MESSAGE_BYTES {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            "Message exceeds 64 MiB",
        ));
    }
    let id = u64::from_be_bytes(header[4..].try_into().unwrap());
    let mut body = vec![0; len];
    reader.read_exact(&mut body)?;
    String::from_utf8(body)
        .map(|body| (id, body))
        .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))
}

pub fn write_frame(writer: &mut impl Write, id: u64, body: &str) -> io::Result<()> {
    if body.len() > MAX_MESSAGE_BYTES {
        return Err(io::Error::new(
            io::ErrorKind::InvalidInput,
            "Message exceeds 64 MiB",
        ));
    }
    let mut header = [0; 12];
    header[..4].copy_from_slice(&(body.len() as u32).to_be_bytes());
    header[4..].copy_from_slice(&id.to_be_bytes());
    writer.write_all(&header)?;
    writer.write_all(body.as_bytes())
}

#[derive(Default)]
struct Inbox {
    messages: VecDeque<Message>,
    pending: HashMap<u64, Option<String>>,
    connected: bool,
    stopped: bool,
    generation: u64,
}

pub struct Endpoint {
    inbox: Mutex<Inbox>,
    changed: Condvar,
    writer: Mutex<Option<TcpStream>>,
    worker: Mutex<Option<JoinHandle<()>>>,
    notify: Box<dyn Fn() + Send + Sync>,
    request_parity: u64,
}

impl Endpoint {
    pub fn new(request_parity: u64, notify: impl Fn() + Send + Sync + 'static) -> Arc<Self> {
        Arc::new(Self {
            inbox: Mutex::new(Inbox::default()),
            changed: Condvar::new(),
            writer: Mutex::new(None),
            worker: Mutex::new(None),
            notify: Box::new(notify),
            request_parity,
        })
    }

    pub fn connect(self: &Arc<Self>, host: &str, port: u16) -> io::Result<()> {
        let mut last_error = io::Error::new(io::ErrorKind::AddrNotAvailable, "No address found");
        for address in (host, port).to_socket_addrs()? {
            match TcpStream::connect_timeout(&address, IO_TIMEOUT) {
                Ok(mut socket) => {
                    socket.set_read_timeout(Some(IO_TIMEOUT))?;
                    let mut handshake = [0; 4];
                    socket.read_exact(&mut handshake)?;
                    if u32::from_be_bytes(handshake) != HANDSHAKE {
                        return Err(io::Error::new(
                            io::ErrorKind::InvalidData,
                            "Invalid server handshake",
                        ));
                    }
                    socket.set_read_timeout(None)?;
                    let generation = self.install(&socket)?;
                    let endpoint = self.clone();
                    *self.worker.lock().unwrap() = Some(thread::spawn(move || {
                        endpoint.receive(&mut socket, generation);
                        endpoint.disconnect(generation, false);
                    }));
                    return Ok(());
                }
                Err(error) => last_error = error,
            }
        }
        Err(last_error)
    }

    pub fn listen(self: &Arc<Self>, host: &str, port: u16) -> io::Result<()> {
        let listener = TcpListener::bind((host, port))?;
        listener.set_nonblocking(true)?;
        let endpoint = self.clone();
        *self.worker.lock().unwrap() = Some(thread::spawn(move || loop {
            if endpoint.inbox.lock().unwrap().stopped {
                break;
            }
            match listener.accept() {
                Ok((mut socket, _)) => {
                    let Ok(generation) = endpoint.install(&socket) else {
                        continue;
                    };
                    if socket.write_all(&HANDSHAKE.to_be_bytes()).is_ok() {
                        endpoint.receive(&mut socket, generation);
                    }
                    endpoint.disconnect(generation, true);
                }
                Err(error) if error.kind() == io::ErrorKind::WouldBlock => {
                    let guard = endpoint.inbox.lock().unwrap();
                    if guard.stopped {
                        break;
                    }
                    drop(
                        endpoint
                            .changed
                            .wait_timeout(guard, Duration::from_millis(10))
                            .unwrap(),
                    );
                }
                Err(_) => break,
            }
        }));
        Ok(())
    }

    fn install(&self, socket: &TcpStream) -> io::Result<u64> {
        // Restore blocking mode: Wine's winsock accepts into an O_NONBLOCK fd
        // (server/sock.c::accept_new_fd) and std::net surfaces that flag, so
        // receive()'s first read_frame would otherwise fail with WouldBlock
        // and the listen loop would immediately disconnect.
        socket.set_nonblocking(false)?;
        socket.set_nodelay(true)?;
        socket.set_write_timeout(Some(IO_TIMEOUT))?;
        let mut writer = self.writer.lock().unwrap();
        let mut inbox = self.inbox.lock().unwrap();
        if inbox.stopped {
            return Err(io::Error::new(
                io::ErrorKind::Interrupted,
                "Endpoint stopped",
            ));
        }
        *writer = Some(socket.try_clone()?);
        inbox.generation += 1;
        inbox.connected = true;
        Ok(inbox.generation)
    }

    fn receive(&self, socket: &mut TcpStream, generation: u64) {
        while let Ok((id, body)) = read_frame(socket) {
            let mut inbox = self.inbox.lock().unwrap();
            if inbox.stopped {
                break;
            }
            if id > 0 && id % 2 == self.request_parity {
                if let Some(response) = inbox.pending.get_mut(&id) {
                    *response = Some(body);
                }
            } else {
                inbox.messages.push_back(Message {
                    id,
                    body,
                    generation,
                });
            }
            self.changed.notify_all();
            drop(inbox);
            (self.notify)();
        }
    }

    fn disconnect(&self, generation: u64, notify: bool) {
        let mut writer = self.writer.lock().unwrap();
        if let Some(socket) = writer.take() {
            let _ = socket.shutdown(Shutdown::Both);
        }
        let mut inbox = self.inbox.lock().unwrap();
        inbox.connected = false;
        if notify && !inbox.stopped {
            inbox.messages.push_back(Message {
                id: 0,
                body: "{\"action\":\"disconnected\"}".into(),
                generation,
            });
        }
        self.changed.notify_all();
        drop(inbox);
        drop(writer);
        (self.notify)();
    }

    pub fn connected(&self) -> bool {
        self.inbox.lock().unwrap().connected
    }

    pub fn generation(&self) -> u64 {
        self.inbox.lock().unwrap().generation
    }

    pub fn send(&self, id: u64, body: &str, generation: u64) -> io::Result<()> {
        let mut writer = self.writer.lock().unwrap();
        let inbox = self.inbox.lock().unwrap();
        if !inbox.connected || inbox.generation != generation || inbox.stopped {
            return Err(io::Error::new(
                io::ErrorKind::NotConnected,
                "Socket is not connected",
            ));
        }
        drop(inbox);
        let result = write_frame(writer.as_mut().unwrap(), id, body);
        if result.is_err() {
            let _ = writer.as_ref().unwrap().shutdown(Shutdown::Both);
        }
        result
    }

    pub fn request(self: &Arc<Self>, id: u64) -> Pending {
        self.inbox.lock().unwrap().pending.insert(id, None);
        Pending {
            endpoint: self.clone(),
            id,
            generation: self.generation(),
        }
    }

    pub fn pop(&self) -> Option<Message> {
        self.inbox.lock().unwrap().messages.pop_front()
    }

    pub fn wait_for_message(&self) -> io::Result<()> {
        let mut inbox = self.inbox.lock().unwrap();
        while inbox.messages.is_empty() {
            if !inbox.connected || inbox.stopped {
                return Err(io::Error::new(
                    io::ErrorKind::NotConnected,
                    "Socket is not connected",
                ));
            }
            inbox = self.changed.wait(inbox).unwrap();
        }
        Ok(())
    }

    pub fn stop(&self) {
        self.inbox.lock().unwrap().stopped = true;
        if let Some(socket) = self.writer.lock().unwrap().take() {
            let _ = socket.shutdown(Shutdown::Both);
        }
        self.changed.notify_all();
        if let Some(worker) = self.worker.lock().unwrap().take() {
            let _ = worker.join();
        }
    }
}

pub struct Pending {
    endpoint: Arc<Endpoint>,
    id: u64,
    pub generation: u64,
}

impl Pending {
    pub fn wait(&self, deadline: Instant) -> io::Result<String> {
        let mut inbox = self.endpoint.inbox.lock().unwrap();
        loop {
            if let Some(body) = inbox.pending.get_mut(&self.id).and_then(Option::take) {
                return Ok(body);
            }
            if inbox.stopped || !inbox.connected || inbox.generation != self.generation {
                return Err(io::Error::new(
                    io::ErrorKind::NotConnected,
                    "Peer disconnected",
                ));
            }
            let remaining = deadline
                .checked_duration_since(Instant::now())
                .ok_or_else(|| io::Error::new(io::ErrorKind::TimedOut, "RPC request timed out"))?;
            inbox = self
                .endpoint
                .changed
                .wait_timeout(inbox, remaining)
                .unwrap()
                .0;
        }
    }

    pub fn poll(&self, deadline: Instant) -> io::Result<Option<String>> {
        let mut inbox = self.endpoint.inbox.lock().unwrap();
        loop {
            if let Some(body) = inbox.pending.get_mut(&self.id).and_then(Option::take) {
                return Ok(Some(body));
            }
            if inbox.stopped || !inbox.connected || inbox.generation != self.generation {
                return Err(io::Error::new(
                    io::ErrorKind::NotConnected,
                    "Peer disconnected",
                ));
            }
            let remaining = deadline
                .checked_duration_since(Instant::now())
                .ok_or_else(|| io::Error::new(io::ErrorKind::TimedOut, "RPC request timed out"))?;
            if !inbox.messages.is_empty() {
                return Ok(None);
            }
            inbox = self
                .endpoint
                .changed
                .wait_timeout(inbox, remaining)
                .unwrap()
                .0;
        }
    }
}

impl Drop for Pending {
    fn drop(&mut self) {
        self.endpoint.inbox.lock().unwrap().pending.remove(&self.id);
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Cursor;

    #[test]
    fn frame_matches_existing_big_endian_protocol() {
        let mut bytes = Vec::new();
        write_frame(&mut bytes, 0x0102030405060708, "abc").unwrap();
        assert_eq!(
            bytes,
            [0, 0, 0, 3, 1, 2, 3, 4, 5, 6, 7, 8, b'a', b'b', b'c']
        );
        assert_eq!(
            read_frame(&mut Cursor::new(bytes)).unwrap(),
            (0x0102030405060708, "abc".into())
        );
    }

    #[test]
    fn rejects_oversized_truncated_and_non_utf8_frames() {
        let mut bytes = vec![0xff; 12];
        assert_eq!(
            read_frame(&mut Cursor::new(&bytes)).unwrap_err().kind(),
            io::ErrorKind::InvalidData
        );
        bytes[..4].copy_from_slice(&2u32.to_be_bytes());
        bytes.push(0xff);
        assert_eq!(
            read_frame(&mut Cursor::new(&bytes)).unwrap_err().kind(),
            io::ErrorKind::UnexpectedEof
        );
        bytes.push(0xff);
        assert_eq!(
            read_frame(&mut Cursor::new(&bytes)).unwrap_err().kind(),
            io::ErrorKind::InvalidData
        );
    }
}
