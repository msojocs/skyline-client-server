const path = require('path')
const p = path.resolve(__dirname, "../../packages/native/build/x86_64-unknown-linux-gnu/main-client.node")
const { mainController } = require(p)

console.info(mainController)

mainController.connect('127.0.0.1', 3002)

const list = mainController.electron.webContents.getAllWebContents()

const target = list[0]
for (const t of ['onBeforeRequest', 'onCompleted']) {
  target.session.webRequest[t](
    { urls: ["<all_urls>"] },
    requestListernerFactory(t),
  )
}
