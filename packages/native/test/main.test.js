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

async function fixture(t) {
  const port = await availablePort();
  const worker = new Worker(path.join(__dirname, 'main-fixture-server.js'), {
    workerData: { webContentsId, port, serverModule: serverPath },
  });
  t.after(async () => { worker.postMessage('stop'); await once(worker, 'exit'); });
  await once(worker, 'message');
  return port;
}

test('main 层 client/server: webContents.fromId 返回可远程调用的代理', { timeout: 15000 }, async t => {
  const port = await fixture(t);
  const { mainController } = require(clientPath);
  t.after(() => mainController.disconnect());

  assert.deepEqual(Object.keys(mainController).sort(), ['connect', 'disconnect', 'electron']);
  mainController.connect('127.0.0.1', port);

  const webContents = mainController.electron.webContents.fromId(webContentsId);
  assert.equal(typeof webContents, 'object');
  assert.equal(webContents.constructor.name, 'WebContents');
  // 同一个真实对象复用同一个代理，返回的是同一个 JS 对象
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

test('未连接与断开后的调用会报错', { timeout: 10000 }, async t => {
  const { mainController } = require(clientPath);
  assert.throws(() => mainController.electron.webContents.fromId(1), /Not connected/);

  const port = await fixture(t);
  mainController.connect('127.0.0.1', port);
  const webContents = mainController.electron.webContents.fromId(webContentsId);
  assert.ok(webContents);
  mainController.disconnect();
  // 断开后取方法本身不报错，调用时才报连接已失效；句柄身份随方法一起摘下来也照样失效。
  const getId = webContents.getId;
  assert.throws(() => getId(), /closed connection/);
  assert.throws(() => mainController.electron.webContents.fromId(webContentsId), /Not connected/);
});
