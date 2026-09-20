const assert = require('node:assert/strict');
const { test } = require('node:test');
const { Worker } = require('node:worker_threads');
const { once } = require('node:events');
const net = require('node:net');
const path = require('node:path');

const directory = process.env.SKYLINE_NATIVE_TEST_DIR || path.resolve(__dirname, '../build',
  process.platform === 'win32' ? 'x86_64-pc-windows-gnu' : 'x86_64-unknown-linux-gnu');
const clientPath = path.join(directory, 'main-client.node');
const serverPath = path.join(directory, 'render-server.node');
const webContentsId = 7;

async function availablePort() {
  const listener = net.createServer();
  listener.listen(0, '127.0.0.1');
  await once(listener, 'listening');
  const port = listener.address().port;
  await new Promise(resolve => listener.close(resolve));
  return port;
}

async function fixture(t, anonymousWebRequest = false) {
  const port = await availablePort();
  const worker = new Worker(path.join(__dirname, 'main-fixture-server.js'), {
    workerData: { webContentsId, port, serverModule: serverPath, anonymousWebRequest },
  });
  t.after(async () => { worker.postMessage('stop'); await once(worker, 'exit'); });
  await once(worker, 'message');
  return port;
}

async function handshakeServer(t, onConnection) {
  const sockets = new Set();
  const listener = net.createServer(socket => {
    sockets.add(socket);
    socket.on('close', () => sockets.delete(socket));
    onConnection?.(socket);
  });
  listener.listen(0, '127.0.0.1');
  await once(listener, 'listening');
  t.after(async () => {
    for (const socket of sockets) socket.destroy();
    await new Promise(resolve => listener.close(resolve));
  });
  return listener;
}

function sendHandshake(socket, value = 114514) {
  const handshake = Buffer.alloc(4);
  handshake.writeUInt32BE(value);
  socket.write(handshake);
}

test('connect returns a Promise and keeps the event loop running during the handshake', { timeout: 10000 }, async t => {
  const { mainController } = require(clientPath);
  t.after(() => mainController.disconnect());
  let accepted = 0;
  let timerRan = false;
  const listener = await handshakeServer(t, socket => {
    accepted++;
    setTimeout(() => {
      timerRan = true;
      sendHandshake(socket);
    }, 25);
  });
  const port = listener.address().port;
  const connecting = mainController.connect('127.0.0.1', port);
  assert.ok(connecting instanceof Promise);
  const concurrent = mainController.connect('127.0.0.1', port);
  // Validation failures must not cancel an otherwise valid connection attempt.
  await assert.rejects(mainController.connect('127.0.0.1', -1), /Port/);
  assert.deepEqual(await Promise.all([connecting, concurrent]), [undefined, undefined]);
  assert.equal(timerRan, true);
  assert.equal(accepted, 1);
  const connected = mainController.connect('127.0.0.1', port);
  assert.ok(connected instanceof Promise);
  assert.equal(await connected, undefined);
  assert.equal(accepted, 1);
});

test('connect rejects invalid arguments and connection failures, and allows retry', { timeout: 10000 }, async t => {
  const { mainController } = require(clientPath);
  t.after(() => mainController.disconnect());
  for (const port of [-1, 65536, 1.5, NaN, '3002']) {
    const connecting = mainController.connect('127.0.0.1', port);
    assert.ok(connecting instanceof Promise);
    await assert.rejects(connecting, /Port|Number/);
  }
  await assert.rejects(mainController.connect(null, 3002));
  const closedPort = await availablePort();
  await assert.rejects(mainController.connect('127.0.0.1', closedPort));
  assert.throws(() => mainController.electron.webContents.fromId(1), /Not connected/);
  const listener = await handshakeServer(t, socket => sendHandshake(socket, 0));
  await assert.rejects(mainController.connect('127.0.0.1', listener.address().port), /Invalid server handshake/);
  const port = await fixture(t);
  await mainController.connect('127.0.0.1', port);
  assert.equal(mainController.electron.webContents.fromId(webContentsId).id, webContentsId);
});

test('disconnect during connect rejects the old attempt without closing a replacement connection', { timeout: 10000 }, async t => {
  const { mainController } = require(clientPath);
  t.after(() => mainController.disconnect());
  const listener = await handshakeServer(t);
  const accepted = once(listener, 'connection');
  const connecting = mainController.connect('127.0.0.1', listener.address().port);
  const rejected = assert.rejects(connecting, /stopped|closed/);
  const [socket] = await accepted;
  mainController.disconnect();

  const port = await fixture(t);
  await mainController.connect('127.0.0.1', port);
  sendHandshake(socket);
  await rejected;
  assert.equal(mainController.electron.webContents.fromId(webContentsId).id, webContentsId);
});

test('main 层 client/server: webContents.fromId 返回可远程调用的代理', { timeout: 15000 }, async t => {
  const port = await fixture(t);
  const { mainController } = require(clientPath);
  t.after(() => mainController.disconnect());

  assert.deepEqual(Object.keys(mainController).sort(), ['connect', 'disconnect', 'electron']);
  await mainController.connect('127.0.0.1', port);

  const webContents = mainController.electron.webContents.fromId(webContentsId);
  assert.equal(typeof webContents, 'object');
  assert.equal(webContents.constructor.name, 'WebContents');
  // 同一个真实对象复用同一个代理，返回的是同一个 JS 对象
  assert.equal(mainController.electron.webContents.fromId(webContentsId), webContents);
  await mainController.connect('127.0.0.1', port);
  assert.equal(mainController.electron.webContents.fromId(webContentsId), webContents);
  // 找不到时是 undefined，而不是抛错
  assert.equal(mainController.electron.webContents.fromId(999), undefined);

  // 属性读取走 dynamicProperty get
  assert.equal(webContents.id, webContentsId);
  assert.equal(webContents.url, `https://example.com/${webContentsId}`);

  // 方法从代理上摘下来单独调用时仍作用在原句柄上（devtools 里 `const getId = webview.getWebContentsId`
  // 那种写法）：句柄身份在取方法时就绑好了，不依赖调用点的 this。
  const { getId, getURL } = webContents;
  assert.equal(getId(), webContentsId);
  assert.equal([webContentsId].map(webContents.getId)[0], webContentsId);
  assert.equal(getURL(), `https://example.com/${webContentsId}`);
  // 接收者不是句柄时给一条明确的错误，而不是 napi 的
  // "Object property '__skylineEpoch' type mismatch. Expect value to be Number, but received Undefined"。
  const getURLDescriptor = Object.getOwnPropertyDescriptor(Object.getPrototypeOf(webContents), 'getURL');
  assert.throws(() => getURLDescriptor.get.call({})(), /not a Skyline instance/);
  // devtools 里"取出来再包一层"的写法：赋值要能盖住原型上的方法，且包起来的那份仍可用。
  webContents.getId = () => getId() + 1;
  assert.equal(webContents.getId(), webContentsId + 1);
  assert.equal(getId(), webContentsId);

  // 方法调用走 dynamic
  assert.equal(await webContents.executeJavaScript('1 + 2'), '1 + 2');
  assert.equal(webContents.reload(), true);
  assert.equal(webContents.isDestroyed(), false);

  // 服务端异步方法落地后才回复
  await webContents.loadURL('https://example.com/next');
  assert.equal(webContents.getURL(), 'https://example.com/next');
  assert.equal(webContents.url, 'https://example.com/next');

  // 插件加载：session / extensions 是属性链上的远端代理，各自按 instanceType 复活成对应类
  const session = webContents.session;
  assert.equal(session.constructor.name, 'Session');
  assert.equal(webContents.session, session);
  const extensions = session.extensions;
  assert.equal(extensions.constructor.name, 'Extensions');
  assert.equal(session.extensions, extensions);
  // loadExtension 返回 Promise，走 ASYNC_METHODS 分支；远端对象 encode 成普通 JSON 直传
  const extension = await extensions.loadExtension('/tmp/fixture-extension');
  assert.deepEqual(extension, { id: 'fixture-1', path: '/tmp/fixture-extension' });
  assert.deepEqual(extensions.getAllExtensions(), [{ id: 'fixture-1', path: '/tmp/fixture-extension' }]);
  assert.equal(extensions.getExtension('fixture-1').path, '/tmp/fixture-extension');
  // 远端的 null 在 client 侧统一解成 undefined（见 binding.rs 的 decode）
  assert.equal(extensions.getExtension('missing'), undefined);
  // 异步失败表现为 Promise reject，而不是同步抛出
  await assert.rejects(extensions.loadExtension(''), /fixture extension path required/);

  // Check the path received by the RPC server, including already mapped URLs.
  for (const [input, expected] of [
    ['file:///home/user/extension', 'file:///Z:/home/user/extension'],
    ['file:///tmp/extension%20目录', 'file:///Z:/tmp/extension%20目录'],
    ['file:///Z:/home/user/extension', 'file:///Z:/home/user/extension'],
    ['file:///C:/extensions/demo', 'file:///C:/extensions/demo'],
    ['Z:/home/user/extension', 'Z:/home/user/extension'],
  ]) {
    assert.equal((await extensions.loadExtension(input)).path, expected);
  }
  assert.equal(await webContents.executeJavaScript('file:///tmp/extension'), 'file:///tmp/extension');

  // 请求拦截：事件名动态取（webRequest[eventName]），监听器是客户端函数——服务端按 callbackId
  // 还原成真实函数（fixture 里监听器不是函数会直接抛错），并且把 Electron 的 callback 代理回客户端。
  const webRequest = session.webRequest;
  assert.equal(webRequest.constructor.name, 'WebRequest');
  assert.equal(session.webRequest, webRequest);

  const urls = [];
  const responses = [];
  webRequest.onBeforeRequest({ urls: ['<all_urls>'] }, (details, callback) => {
    urls.push(details.url);
    assert.equal(typeof callback, 'function');
    // callback 是服务端传回来的函数代理，调用它的返回值就是服务端监听器回调的返回值。
    responses.push(callback({ cancel: true }));
  });
  assert.deepEqual(urls, ['https://example.com/onBeforeRequest']);
  assert.deepEqual(responses, [
    { accepted: true, name: 'onBeforeRequest', response: { cancel: true } },
  ]);

  // 稍后再调用 callback 也要回到服务端：callback 代理是惰性的，不要求在监听器这一次调用内完成。
  webRequest.onCompleted({ urls: ['*'] }, (details, callback) => {
    setImmediate(() => responses.push(callback({ ok: true })));
  });
  const deadline = Date.now() + 5000;
  while (responses.length < 2 && Date.now() < deadline) {
    await new Promise((resolve) => setTimeout(resolve, 10));
  }
  assert.deepEqual(responses[1], {
    accepted: true,
    name: 'onCompleted',
    response: { ok: true },
  });

  // executeJavaScript 沿用 render client 的 AsyncTask 语义（不阻塞 JS 线程），
  // 所以服务端的失败表现为 Promise reject，而不是同步抛出。
  await assert.rejects(webContents.executeJavaScript('reject'), /fixture async failure/);

  // 其它方法的同步失败要穿过 RPC 直接抛回 client
  await webContents.loadURL('boom');
  assert.throws(() => webContents.getURL(), /fixture sync failure/);

  // 代理面由类表限定：main 侧的类不挂 webview 元素的那套方法
  assert.equal(typeof webContents.bogus, 'undefined');
  assert.equal(typeof webContents.setAttribute, 'undefined');
  assert.equal(typeof webContents.isConnected, 'undefined');
});

test('WebContents.once forwards event arguments and fires each listener only once', { timeout: 15000 }, async t => {
  const port = await fixture(t);
  const { mainController } = require(clientPath);
  t.after(() => mainController.disconnect());
  await mainController.connect('127.0.0.1', port);

  const webContents = mainController.electron.webContents.fromId(webContentsId);
  const navigations = [];
  const { once: listenOnce } = webContents;
  assert.equal(listenOnce('did-navigate', (...args) => navigations.push(args)), webContents);
  await webContents.loadURL('https://example.com/first');
  await webContents.loadURL('https://example.com/second');
  assert.deepEqual(navigations, [[{}, 'https://example.com/first', 200, 'OK']]);

  const destroyed = [];
  const listener = (...args) => destroyed.push(args);
  assert.equal(webContents.once('destroyed', listener).once('destroyed', listener), webContents);
  assert.deepEqual(destroyed, []);
  webContents.close();
  assert.equal(webContents.isDestroyed(), true);
  assert.deepEqual(destroyed, [[], []]);
  // The fixture emits on every close to check that once removed both registrations.
  webContents.close();
  assert.deepEqual(destroyed, [[], []]);
});

test('anonymous Electron webRequest returns a callable object', { timeout: 15000 }, async t => {
  const port = await fixture(t, true);
  const { mainController } = require(clientPath);
  t.after(() => mainController.disconnect());
  await mainController.connect('127.0.0.1', port);

  const [target] = mainController.electron.webContents.getAllWebContents();
  const webRequest = target.session.webRequest;
  assert.equal(typeof webRequest, 'object');
  assert.ok(webRequest instanceof Object);
  assert.equal(webRequest.constructor.name, 'Object');
  assert.equal(target.session.webRequest, webRequest);
  assert.equal(mainController.electron.webContents.fromId(webContentsId).session.webRequest, webRequest);

  for (const name of [
    'onBeforeRequest', 'onBeforeSendHeaders', 'onSendHeaders', 'onHeadersReceived',
    'onResponseStarted', 'onBeforeRedirect', 'onCompleted', 'onErrorOccurred',
  ]) {
    assert.equal(typeof webRequest[name], 'function');
  }
  const responses = [];
  const { onBeforeRequest } = webRequest;
  onBeforeRequest({ urls: ['<all_urls>'] }, (details, callback) => {
    assert.equal(details.url, 'https://example.com/onBeforeRequest');
    responses.push(callback({ cancel: true }));
  });
  assert.deepEqual(responses, [
    { accepted: true, name: 'onBeforeRequest', response: { cancel: true } },
  ]);
  assert.equal(webRequest.requestId, 1);
  assert.equal(webRequest.missing, undefined);
  assert.equal(webRequest[Symbol.toStringTag], undefined);
  assert.equal(webRequest.onBeforeRequest, onBeforeRequest);
  webRequest.onBeforeRequest = () => 'local override';
  assert.equal(webRequest.onBeforeRequest(), 'local override');

  mainController.disconnect();
  assert.throws(() => onBeforeRequest({}, () => {}), /closed connection/);
  assert.throws(() => webRequest.requestId, /closed connection/);
});

test('未连接与断开后的调用会报错', { timeout: 10000 }, async t => {
  const { mainController } = require(clientPath);
  assert.throws(() => mainController.electron.webContents.fromId(1), /Not connected/);

  const port = await fixture(t);
  await mainController.connect('127.0.0.1', port);
  const webContents = mainController.electron.webContents.fromId(webContentsId);
  assert.ok(webContents);
  mainController.disconnect();
  // 断开后取方法本身不报错，调用时才报连接已失效；句柄身份随方法一起摘下来也照样失效。
  const getId = webContents.getId;
  assert.throws(() => getId(), /closed connection/);
  assert.throws(() => mainController.electron.webContents.fromId(webContentsId), /Not connected/);

  // The same server instance must be usable after a client reconnects: remote IDs
  // from the previous connection must not be reused from stale WeakMap entries.
  await mainController.connect('127.0.0.1', port);
  const reconnected = mainController.electron.webContents.fromId(webContentsId);
  assert.equal(reconnected.id, webContentsId);
  mainController.disconnect();
});
