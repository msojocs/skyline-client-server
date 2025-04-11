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
  return new Uint8Array(10);
});
shell.setLoadResourceAsyncCallback((...args) => {
  console.info("setLoadResourceAsyncCallback", ...args);
});
shell.setHttpRequestCallback((...args) => {
  console.info("setHttpRequestCallback", ...args);
});
let ready = false;
shell.setNotifyWindowReadyCallback((...args) => {
  // args: windowId
  console.info("setNotifyWindowReadyCallback", ...args);
  ready = true;
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
const sharedMemory = require("D:/github/shared-memory/build/sharedMemory.node");
const key = "test";
const buffer = sharedMemory.setMemory(key, 21066248);
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
  backgroundColorContent: "#FFFFFFFF",
});

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

(async () => {
  while (!ready) {
    await sleep(100);
  }
  console.info("window ready");

  const page = new global.SkylineGlobal.PageContext(1, 1, {
    defaultBlockLayout: true,
    defaultContentBox: false,
    enableImagePreload: false,
    enableScrollViewAutoSize: false,
    tagNameStyleIsolation: 0,
  });
  const group = page.createStyleSheetIndexGroup();
  page.appendStyleSheetIndex("wxlibfile://WAStyleIndex.fpiib", group);
  page.appendCompiledStyleSheets([
    {
      groupId: 0,
      path: "wxlibfile://WASkylineStyle",
      prefix: "",
      scopeId: 0,
      styleSheet: {
        binary: true,
        enabled: true,
        index: 0,
        injected: 2,
        path: "wxlibfile://WASkylineStyle",
        priority: 3,
        styleScope: 0,
        styleScopeSetter: null,
      },
    },
    {
      groupId: 0,
      path: "wxlibfile://WASkylineStyleV2",
      prefix: "",
      scopeId: 0,
      styleSheet: {
        binary: true,
        enabled: true,
        index: 1,
        injected: 2,
        path: "wxlibfile://WASkylineStyleV2",
        priority: 3,
        styleScope: 0,
        styleScopeSetter: null,
      },
    },
  ]);

  console.info('appendChild test')
  const parent = page.createElement("view", "view")
  const child = page.createElement("view", "view")
  parent.appendChild(child)
})();
