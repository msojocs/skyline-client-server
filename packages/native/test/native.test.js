const assert = require('node:assert/strict');
const { test } = require('node:test');
const { Worker } = require('node:worker_threads');
const { once } = require('node:events');
const net = require('node:net');
const path = require('node:path');
const { spawnSync } = require('node:child_process');

const directory = process.env.SKYLINE_NATIVE_TEST_DIR || path.resolve(__dirname, '../build',
  process.platform === 'win32' ? 'x86_64-pc-windows-gnu' : 'x86_64-unknown-linux-gnu');
const clientPath = path.join(directory, 'render-client.node');
const serverPath = path.join(directory, 'render-server.node');

async function availablePort() {
  const listener = net.createServer();
  listener.listen(0, '127.0.0.1');
  await once(listener, 'listening');
  const port = listener.address().port;
  await new Promise(resolve => listener.close(resolve));
  return port;
}

async function fixture(t) {
  const port = await availablePort();
  const worker = new Worker(path.join(__dirname, 'fixture-server.js'), { workerData: { module: serverPath, port } });
  t.after(async () => { worker.postMessage('stop'); await once(worker, 'exit'); });
  await once(worker, 'message');
  return port;
}

test('exports, argument validation, and idle environment cleanup', { timeout: 10000 }, async () => {
  const client = require(clientPath);
  const server = require(serverPath);
  assert.deepEqual(Object.keys(client), ['Controller']);
  assert.deepEqual(Object.keys(server).sort(), ['blockUntilNextMessage', 'sendMessageSingle', 'sendMessageSync', 'setMessageCallback', 'start', 'stop']);
  assert.throws(() => new client.Controller(), /function|argument/i);
  assert.throws(() => client.Controller.connect('localhost', -1), /Port/);
  assert.throws(() => server.start('localhost', 1.5), /Port/);
  assert.throws(() => server.setMessageCallback(42), /function/i);
  assert.throws(() => server.sendMessageSingle('hello'), /connected/i);
  let reported;
  assert.throws(() => new client.Controller(message => { reported = message; }), /connected/i);
  assert.match(reported, /connected/i);
  client.Controller.disconnect();
  server.stop();
  const serverChild = spawnSync(process.execPath, ['-e', `const s=require(${JSON.stringify(serverPath)});s.start('127.0.0.1',0);`], { timeout: 5000, encoding: 'utf8' });
  assert.equal(serverChild.status, 0, serverChild.stderr || serverChild.error?.message);

  const port = await availablePort();
  server.start('127.0.0.1', port);
  try {
    const clientChild = spawnSync(process.execPath, ['-e',
      `require(${JSON.stringify(clientPath)}).Controller.connect('127.0.0.1',${port});`],
    { timeout: 5000, encoding: 'utf8' });
    assert.equal(clientChild.status, 0, clientChild.stderr || clientChild.error?.message);
  } finally { server.stop(); }
});

test('Controller dialog callback forwards type and arguments through a nested RPC', { timeout: 10000 }, async t => {
  const port = await fixture(t);
  const { Controller } = require(clientPath);
  t.after(() => Controller.disconnect());
  Controller.connect('127.0.0.1', port);
  const controller = new Controller(() => {});
  const webview = controller.webview;
  const dialogCallbackResult = new Promise((resolve, reject) => {
    const callback = (target, requestId, type, ...args) => {
      try {
        assert.equal(target, webview);
        assert.equal(requestId, 'dialog-1');
        assert.equal(type, 'alert');
        assert.deepEqual(args, ['customer service']);
        assert.equal(controller.resolveDialog(requestId, ''), true);
        resolve(requestId);
      } catch (error) {
        reject(error);
      }
    };
    callback.__asyncCallback = true;
    assert.equal(controller.setDialogCallback(callback), undefined);
  });
  assert.equal(controller.dialog(webview, {
    requestId: 'dialog-1', type: 'alert', args: ['customer service'],
  }), true);
  assert.equal(await dialogCallbackResult, 'dialog-1');
});

test('Rust client/server RPC, objects, callbacks, and nested synchronous calls', { timeout: 30000 }, async t => {
  const port = await fixture(t);
  const { Controller } = require(clientPath);
  t.after(() => Controller.disconnect());
  Controller.connect('127.0.0.1', port);
  const errors = [];
  const controller = new Controller(message => errors.push(message));
  assert.equal(controller.constructor.name, 'Controller');
  const webview = controller.webview;
  assert.equal(webview.constructor.name, 'WebviewElement');
  assert.equal(webview, controller.webview);
  webview.src = 'https://example.com/';
  assert.equal(webview.src, 'https://example.com/');
  webview.style.display = 'block';
  webview.style.pointerEvents = 'none';
  assert.equal(webview.style.display, 'block');
  assert.equal(webview.style.pointerEvents, 'none');
  webview.setAttribute('sample', 'value');
  assert.equal(webview.getAttribute('sample'), 'value');
  webview.removeAttribute('sample');
  assert.equal(webview.getAttribute('sample'), undefined);
  assert.equal(controller.mount(), undefined);
  assert.equal(webview.reload(), true);
  assert.throws(() => webview.showDevTools(), /Not implemented/);
  const rejectedExecution = webview.executeJavaScript({ error: true });
  assert.equal(typeof rejectedExecution.then, 'function');
  await assert.rejects(rejectedExecution, /fixture remote error/);
  assert.ok(errors.includes('fixture remote error'));

  const callback = value => value;
  for (const key of ['onAuthRequired', 'onMessage']) {
    const event = webview.request[key];
    event.addListener(callback);
    assert.equal(event.hasListener(callback), true);
    event.removeListener(callback);
    assert.equal(event.hasListener(callback), false);
  }
  const rules = webview.request.onRequest;
  rules.addRules([{ id: 'test' }]);
  assert.deepEqual(rules.getRules(), [{ id: 'test' }]);
  rules.removeRules();
  assert.deepEqual(rules.getRules(), []);

  const echo = value => webview.executeJavaScript({ echo: true, value });
  assert.deepEqual(await echo({ text: '中文', array: [1, true, null], buffer: Buffer.from([0, 128, 255]),
    arrayBuffer: Uint8Array.from([1, 2, 3]).buffer }),
  { text: '中文', array: [1, true, undefined], buffer: [0, 128, 255], arrayBuffer: [1, 2, 3] });
  assert.deepEqual(await echo(Buffer.alloc(0)), []);
  assert.deepEqual(await echo(new ArrayBuffer(0)), []);
  assert.deepEqual(await echo({ target: webview }), { target: { instanceId: webview.instanceId } });
  const circular = {}; circular.self = circular;
  await assert.rejects(echo(circular), /circular/);
  const repeated = {};
  assert.deepEqual(await echo([repeated, repeated]), [{}, {}]);
  const protoKey = await echo(JSON.parse('{"__proto__":{"safe":true}}'));
  assert.equal(Object.getPrototypeOf(protoKey), Object.prototype);
  assert.deepEqual(protoKey.__proto__, { safe: true });

  const worklet = () => 1;
  Object.assign(worklet, { __worklet: true, __workletHash: 123, __location: 'test', asString: '() => 1', _closure: { value: 5 } });
  assert.deepEqual(await echo(worklet), { callbackId: worklet.__callbackId, asyncCallback: false, __worklet: true,
    __workletHash: 123, __location: 'test', asString: '() => 1', _closure: { value: 5 } });
  const remoteFunction = await webview.executeJavaScript({ function: true });
  assert.deepEqual(remoteFunction('value', 5), { called: '71', params: ['value', 5] });
  const returnedController = await webview.executeJavaScript({ controller: true });
  assert.equal(returnedController instanceof Controller, true);

  const nested = await webview.executeJavaScript({ args: [3] }, value => {
    assert.equal(webview.src, 'https://example.com/');
    assert.equal(webview.reload(), true);
    return value * 2;
  });
  assert.equal(nested, 6);
  assert.equal(await webview.executeJavaScript({ args: [7] }, Object.freeze(value => value + 1)), 8);
  let invoked = 0;
  let asyncExecution;
  const asyncResult = new Promise(resolve => {
    const callback = value => { invoked++; resolve(value); };
    callback.__asyncCallback = true;
    asyncExecution = webview.executeJavaScript({ later: true, args: ['async'] }, callback);
  });
  assert.equal(typeof asyncExecution.then, 'function');
  assert.equal(await asyncExecution, undefined);
  assert.equal(await asyncResult, 'async');
  await new Promise(resolve => setTimeout(resolve, 25));
  assert.equal(invoked, 1);
  let syncExecution;
  const syncResult = new Promise(resolve => {
    syncExecution = webview.executeJavaScript({ later: true, args: ['idle-sync'] }, value => { resolve(value); return 'reply'; });
  });
  assert.equal(await syncExecution, undefined);
  assert.equal(await syncResult, 'idle-sync');
  assert.equal(await webview.executeJavaScript({ blockUntilNext: true }), true);
  assert.equal(webview.getUserAgent(), 'Fixture');

  Controller.disconnect();
  Controller.connect('127.0.0.1', port);
  assert.throws(() => webview.reload(), /closed connection/);
  assert.throws(() => remoteFunction(), /closed connection/);
  assert.ok(new Controller(() => {}).webview);
});

test('wire format, fragmented frames, disconnect notification, and server restart', { timeout: 15000 }, async () => {
  const server = require(serverPath);
  const port = await availablePort();
  const messages = [];
  server.setMessageCallback((body, id) => {
    messages.push([body, id]);
    if (id) server.sendMessageSingle('{"result":true}', id);
  });
  server.start('127.0.0.1', port);
  assert.throws(() => server.start('127.0.0.1', port), /already started/);
  const socket = net.connect(port, '127.0.0.1');
  const received = [];
  socket.on('data', data => received.push(data));
  try {
    await once(socket, 'data');
    assert.equal(Buffer.concat(received).readUInt32BE(), 114514);
    received.length = 0;
    const payload = Buffer.from('{"type":"test"}');
    const header = Buffer.alloc(12);
    header.writeUInt32BE(payload.length);
    header.writeBigUInt64BE(9007199254740989n, 4);
    const response = once(socket, 'data');
    for (const byte of Buffer.concat([header, payload])) socket.write(Buffer.from([byte]));
    await response;
    while (Buffer.concat(received).length < 27) await once(socket, 'data');
    const frame = Buffer.concat(received);
    assert.equal(frame.readUInt32BE(), 15);
    assert.equal(frame.readBigUInt64BE(4), 9007199254740989n);
    assert.equal(frame.subarray(12).toString(), '{"result":true}');
    assert.deepEqual(messages[0], [payload.toString(), 9007199254740989]);
    socket.destroy();
    await new Promise(resolve => setTimeout(resolve, 30));
    assert.equal(messages.at(-1)[0], '{"action":"disconnected"}');
  } finally { socket.destroy(); server.stop(); }
  server.start('127.0.0.1', port);
  server.stop();
});

// devtools 的 preload 会把方法从句柄上摘出来再包一层：
//   const getId = webview.getWebContentsId
//   webview.getWebContentsId = () => 114514 + getId()
// 摘下来的方法必须仍然作用在原句柄上，否则 this 丢失后会抛 napi 的
// "Object property '__skylineEpoch' type mismatch. Expect value to be Number, but received Undefined"。
test('句柄方法摘下来调用、被包一层之后仍作用在原句柄上', { timeout: 10000 }, async t => {
  const port = await fixture(t);
  const { Controller } = require(clientPath);
  t.after(() => Controller.disconnect());
  Controller.connect('127.0.0.1', port);
  const webview = new Controller(() => {}).webview;

  const getUserAgent = webview.getUserAgent;
  assert.equal(getUserAgent(), 'Fixture');
  assert.equal([0].map(webview.getUserAgent)[0], 'Fixture');
  assert.equal(webview.getUserAgent(), 'Fixture');

  // 包一层要能盖住原型上的方法，被包住的那份仍然可用
  webview.getUserAgent = () => `${getUserAgent()}!`;
  assert.equal(webview.getUserAgent(), 'Fixture!');

  // 接收者不是句柄时给明确错误，而不是 napi 的属性类型不匹配
  const descriptor = Object.getOwnPropertyDescriptor(Object.getPrototypeOf(webview), 'getUserAgent');
  assert.throws(() => descriptor.get.call({})(), /not a Skyline instance/);

  // 读方法本身不报错，调用才报连接已失效
  Controller.disconnect();
  const reload = webview.reload;
  assert.throws(() => reload(), /closed connection/);
});
