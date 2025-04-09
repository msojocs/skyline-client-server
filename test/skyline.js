const skylineClient = require("../build/skylineClient.node");

console.info("1111");
console.info("Using global.SkylineDebugInfo:");
console.info(global.SkylineDebugInfo);
console.info(global.SkylineDebugInfo);
const shell = new global.SkylineShell();
console.info(shell);
shell.setNotifyBootstrapDoneCallback((...args) => {
  console.info("setNotifyBootstrapDoneCallback", ...args);
});
shell.setSafeAreaEdgeInsets(0, 47, 0, 34);
shell.setLoadResourceCallback((...args) => {
  console.info("setLoadResourceCallback", ...args);
});
shell.setLoadResourceAsyncCallback((...args) => {
  console.info("setLoadResourceAsyncCallback", ...args);
});
shell.setHttpRequestCallback((...args) => {
  console.info("setHttpRequestCallback", ...args);
});

shell.setNotifyWindowReadyCallback((...args) => {
  // args: windowId
  console.info("setNotifyWindowReadyCallback", ...args);
});
shell.setNotifyRouteDoneCallback((...args) => {
  console.info("setNotifyRouteDoneCallback", ...args);
});
shell.setNavigateBackCallback((...args) => {
  console.info("setNavigateBackCallback", ...args);
});
shell.setNavigateBackDoneCallback((...args) => {
  console.info("setNavigateBackDoneCallback", ...args);
});
shell.setSendLogCallback((...args) => {
  console.info("sendLogCallback", ...args);
});
const sharedMemory = require('D:/github/shared-memory/build/sharedMemory.node')
const key = "test"
const buffer = sharedMemory.setMemory(key, 21066248)
const win = shell.createWindow(
  1,
  "D:/down/nwjs-sdk-v0.54.1-win-x64/package.nw/node_modules/skyline-addon/bundle",
  390,
  844,
  2,
  true,
  key,
  "D:/down/nwjs-sdk-v0.54.1-win-x64/package.nw/node_modules/skyline-addon/build/skyline.node"
);

  shell.notifyAppLaunch(1, 1, {
    backgroundColorContent: '#FFFFFFFF',
  })
setTimeout(() => {
      
}, 500)