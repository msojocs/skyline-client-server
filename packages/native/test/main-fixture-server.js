// main.test.js 的服务端 fixture：在 Worker 里跑 main-rpc.js，注入假的 electron。
// 独立的 Worker 有自己的事件循环，返回 Promise 的方法才有机会 resolve（client 侧是同步阻塞的）。
const { parentPort, workerData } = require('node:worker_threads');
const { createMainRpc } = require('../../electron/main-rpc.js');

// 两个约束：
// 1. 必须是个类实例——普通对象会被 main-rpc.js 当作 JSON 数据直传，而不是代理成远端实例；
// 2. 类名必须是 `WebContents`——远端类名取 constructor.name，要和 main_client.rs 的
//    CLASSES 表里 `wire_name: "WebContents"` 对上，否则 client 侧 remote() 复活不出代理。
class WebContents {
  constructor(id) {
    this.id = id;
    this.url = `https://example.com/${id}`;
    this.destroyed = false;
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
