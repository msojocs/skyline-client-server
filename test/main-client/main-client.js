const path = require('path')
const p = path.resolve(__dirname, "../../packages/native/build/x86_64-unknown-linux-gnu/main-client.node")
const { mainController } = require(p)

console.info(mainController)

mainController.connect('127.0.0.1', 3002)

console.info('mainController:', mainController)
console.info('webContents', mainController.electron.webContents)
console.info('webContents fromId(0):', mainController.electron.webContents.fromId(0))
console.info('webContents getAllWebContents():', mainController.electron.webContents.getAllWebContents())

// 插件加载：属性链上的对象（session / extensions）都是远端代理，loadExtension 是异步方法。
async function loadExtension(extensionPath) {
  const target = mainController.electron.webContents.fromId(0)
  const extension = await target.session.extensions.loadExtension(extensionPath)
  console.info('loadExtension:', extension)
  console.info('getAllExtensions:', target.session.extensions.getAllExtensions())
}

// 请求拦截：事件名动态取（webRequest[t]）。监听器是本地函数，服务端按 callbackId 还原成真实
// 函数交给 Electron；监听器拿到的第二个参数是 Electron 的 callback，被代理回本地，可稍后再调用。
function requestListernerFactory(t) {
  return (details, callback) => {
    console.info('webRequest', t, details.url)
    callback?.({})
  }
}

function interceptRequests() {
  const target = mainController.electron.webContents.fromId(0)
  for (const t of ['onBeforeRequest', 'onCompleted']) {
    target.session.webRequest[t](
      { urls: ["<all_urls>"] },
      requestListernerFactory(t),
    )
  }
}

if (process.argv[2]) {
  loadExtension(process.argv[2]).catch((error) => console.error(error))
}
if (process.argv[3] === 'webrequest') {
  interceptRequests()
}