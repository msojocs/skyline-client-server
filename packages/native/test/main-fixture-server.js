// main.test.js 的服务端 fixture：在 Worker 里跑 main-rpc.js，注入假的 electron。
// 独立的 Worker 有自己的事件循环，返回 Promise 的方法才有机会 resolve（client 侧是同步阻塞的）。
const { parentPort, workerData } = require('node:worker_threads');
const { createMainRpc } = require('../../electron/main-rpc.js');

// 类实例编码成远端句柄，普通对象递归编码为 JSON。
// 命名类对应 main_client.rs 的 CLASSES 表；匿名类以 Object 句柄交给客户端动态读取成员。
class Session {
  constructor() {
    this.extensions = new Extensions();
    this.webRequest = new WebRequest();
  }
}

// 模拟 Electron 的 WebRequest：注册后立刻用一份 details 触发监听器（真实 Electron 里触发
// 时机由请求决定）。监听器调用 callback 的返回值经 functionData 代理回到客户端，测试据此断言
// 回调确实穿过 RPC 落到了服务端。
class WebRequest {
  constructor() {
    this.listeners = new Map();
    this.requestId = 0;
  }
  invoke(name, url) {
    const listener = this.listeners.get(name);
    if (!listener) return;
    listener({ id: ++this.requestId, url, webContentsId: 7 }, (response) => {
      return { accepted: true, name, response };
    });
  }
}

// Electron 36 的 WebRequest 使用匿名原生构造函数，不能依赖 constructor.name 识别。
if (workerData.anonymousWebRequest) {
  Object.defineProperty(WebRequest, 'name', { value: '' });
}

// Electron 的八个事件名，签名都是 (filter, listener)。
for (const name of [
  'onBeforeRequest',
  'onBeforeSendHeaders',
  'onSendHeaders',
  'onHeadersReceived',
  'onResponseStarted',
  'onBeforeRedirect',
  'onCompleted',
  'onErrorOccurred',
]) {
  WebRequest.prototype[name] = function (filter, listener) {
    this.listeners.set(name, listener);
    // 监听器必须是真实函数：客户端传的是 {callbackId}，main-rpc.js 要把它还原回来。
    if (typeof listener !== 'function') throw new Error(`fixture listener is not a function: ${name}`);
    this.invoke(name, `https://example.com/${name}`);
  };
}

class Extensions {
  constructor() {
    this.loaded = [];
  }
  loadExtension(path) {
    // Electron 的 loadExtension 返回 Promise；client 侧走异步分支，失败表现为 reject。
    if (!path) return Promise.reject(new Error('fixture extension path required'));
    return Promise.resolve().then(() => {
      const extension = { id: `fixture-${this.loaded.length + 1}`, path };
      this.loaded.push(extension);
      return extension;
    });
  }
  getAllExtensions() {
    return this.loaded;
  }
  getExtension(extensionId) {
    return this.loaded.find((extension) => extension.id === extensionId) ?? null;
  }
  removeExtension(extensionId) {
    this.loaded = this.loaded.filter((extension) => extension.id !== extensionId);
  }
}

class WebContents {
  constructor(id) {
    this.id = id;
    this.url = `https://example.com/${id}`;
    this.destroyed = false;
    this.session = new Session();
  }
  loadURL(url) {
    return Promise.resolve().then(() => { this.url = url; });
  }
  getURL() {
    // 同步抛出的失败路径：client 侧应表现为同步 throw。
    if (this.url === 'boom') throw new Error('fixture sync failure');
    return this.url;
  }
  getId() {
    return this.id;
  }
  isDestroyed() {
    return this.destroyed;
  }
  reload() {
    return true;
  }
  close() {
    this.destroyed = true;
  }
  openDevTools() {
    return undefined;
  }
  executeJavaScript(script) {
    // 异步失败的路径：client 侧表现为 Promise reject。
    if (script === 'reject') return Promise.reject(new Error('fixture async failure'));
    return Promise.resolve(script);
  }
}

const webContents = new WebContents(workerData.webContentsId);

const rpc = createMainRpc({
  electron: {
    webContents: {
      fromId: (id) => (id === workerData.webContentsId ? webContents : undefined),
      getAllWebContents: () => [webContents],
    },
  },
  server: require(workerData.serverModule),
  port: workerData.port,
});

rpc.start();
parentPort.postMessage('ready');
parentPort.on('message', (message) => {
  if (message === 'stop') {
    rpc.stop();
    parentPort.close();
  }
});
