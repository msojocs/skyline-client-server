const path = require('path')
const p = path.resolve(__dirname, "../packages/native/build/x86_64-unknown-linux-gnu/main-client.node")
const { mainController } = require(p)

console.info(mainController)

mainController.connect('127.0.0.1', 3002)

console.info('mainController:', mainController)
console.info('webContents', mainController.electron.webContents)
console.info('webContents', mainController.electron.webContents.fromId(0))

// 插件加载：属性链上的对象（session / extensions）都是远端代理，loadExtension 是异步方法。
async function loadExtension(extensionPath) {
  const target = mainController.electron.webContents.fromId(0)
  const extension = await target.session.extensions.loadExtension(extensionPath)
  console.info('loadExtension:', extension)
  console.info('getAllExtensions:', target.session.extensions.getAllExtensions())
}

if (process.argv[2]) {
  loadExtension(process.argv[2]).catch((error) => console.error(error))
}