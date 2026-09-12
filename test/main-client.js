const path = require('path')
const p = path.resolve(__dirname, "../packages/native/build/x86_64-unknown-linux-gnu/main-client.node")
const { mainController } = require(p)

console.info(mainController)

mainController.connect('127.0.0.1', 3002)

console.info('mainController:', mainController)
console.info('webContents', mainController.electron.webContents)
console.info('webContents', mainController.electron.webContents.fromId(0))