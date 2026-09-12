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

  // 方法调用走 dynamic
  assert.equal(await webContents.executeJavaScript('1 + 2'), '1 + 2');
  assert.equal(webContents.reload(), true);
  assert.equal(webContents.isDestroyed(), false);

  // 服务端异步方法落地后才回复
  await webContents.loadURL('https://example.com/next');
  assert.equal(webContents.getURL(), 'https://example.com/next');
  assert.equal(webContents.url, 'https://example.com/next');

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
  assert.ok(mainController.electron.webContents.fromId(webContentsId));
  mainController.disconnect();
  assert.throws(() => mainController.electron.webContents.fromId(webContentsId), /Not connected/);
});
