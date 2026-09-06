const path = require('path');
const clientNode = process.env['SKYLINE_DEV_PATH']
    ? `${process.env['SKYLINE_DEV_PATH']}/skyline.node` 
    : path.resolve(__dirname, "../packages/native/build/skyline.node")

const skylineClient = require(clientNode);
console.info("skylineClient", skylineClient)
console.info("skylineClient.Controller.connect", skylineClient.Controller.connect)
skylineClient.Controller.connect()
const log = console.info
const controller = new skylineClient.Controller(log)
console.log("skylineClient", controller)
const wv = controller.webview
console.log("wv", wv)
console.log("src:", wv.src)
wv.src = "https://www.baidu.com";
console.log("src:", wv.src)
wv.style.display = 'block'
console.log("style:", wv.style.display)
controller.mount()