# Skyline Wine 环境 `__messager__` 缺失根因报告

## 1. 结论

Skyline (Wine) 下报 `Cannot read properties of undefined (reading '__messager__')`，不是 `__global__` 没在源码里赋值，而是 **wx devtools 把 preload 路径生成在原生 Linux 侧，路径里没有 Wine 要求的 `Z:` 盘符**，传到 Skyline Server (Wine) 的 webview 上时 Electron 的 `file://` handler 直接拒绝加载，preload 脚本整个没跑起来。

    期望路径（Wine 可解析）：file:///Z:/home/msojocs/.../inject/preload/electron/index.js
    实际路径（wx devtools 生成）：file:///home/msojocs/.../inject/preload/electron/index.js

preload 没跑 → `window.__global__` 没注入 → `EnvMessagerService` 构造函数在 `appservice/index.js:6620` 取 `window.parent.__global__.__messager__` 时对 `undefined` 取属性抛错。

## 2. 验证证据（通过 9222 实测）

### 2.1 appservice 页面里的全局状态

在 `50DAD4CC47DA7DA5079B38876E103B91`（appservice guest renderer）执行：

    typeof window.process          // "undefined"
    typeof window.require          // "undefined"
    typeof window.module           // "undefined"
    typeof window.electron         // "undefined"
    typeof window.Messager         // "undefined"
    typeof window.__global__       // "undefined"   ← 构造函数要的就是这个
    window.__global                // object (65 keys, 含 envMessager)
    Object.getOwnPropertyNames(window).filter(k => k.startsWith('__'))
      // ["__WeixinJSContext","__contextSupport","__devtoolsConfig",
      //  "__global","__subcontext_ready__","__subcontext_ready_to_evaluate__","__wxConfig"]

`window.__global` 是有的（documentstart 注入），`window.__global__` 没有（preload 才注入）。

### 2.2 host page 里实际有两个 webview

在 `E944E4973041B06620790F1D010DEDC6`（Skyline Server host page）执行 `document.querySelectorAll('webview')`：

    [
      { id: "devtool",     preload: null, partition: "trusted",          src: null },
      { id: "appservice",  preload: "file:///home/msojocs/.../inject/preload/electron/index.js",
        partition: "persist:skyline_appservice_0",
        src: "http://127.0.0.1:48640/appservice/s0/_sessionId/.../mainframe",
        webpreferences: "webSecurity=no,backgroundThrottling=no, contextIsolation=false,enableBlinkFeatures=SharedArrayBuffer",
        nodeintegration: "true", nodeintegrationinsubframes: "" }
    ]

`#devtool` 是 `index.html` 里的静态占位，没 preload；`#appservice` 才是真正承载 appservice 的 webview，preload 已设值——但路径错。

### 2.3 在 host renderer 里直接 fetch 验证路径

| URL | 结果 |
|---|---|
| `file:///home/msojocs/github/wechat-web-devtools-linux/resources/app/js/extensions/inject/preload/electron/index.js` | **Failed to fetch** |
| `file:///Z:/home/msojocs/github/wechat-web-devtools-linux/resources/app/js/extensions/inject/preload/electron/index.js` | 200, 109444 bytes |

Wine 下 Electron 只认带 `Z:` 盘符的 `file://` URL，不带盘符的 Linux 路径直接拒。

## 3. 错误链路

```
appservice/mainframe                                  http://127.0.0.1:48640/appservice/s0/.../mainframe
  └─ ide:///extensions/appservice/index.js:5421
       └─ new r.EnvMessagerService()                    ← 实例化
            └─ index.js:6620:65
                 const t = window.__global__?.__messager__
                         || window.parent.__global__.__messager__
                 两者都 undefined，对 undefined 取 .__messager__ 抛错
```

关键代码 [appservice/index.js:6609-6640](/home/msojocs/github/wechat-web-devtools-linux/resources/app/js/extensions/appservice/index.js:6609)：

```js
t.EnvMessagerService = class {
  constructor() {
    var e;
    ((this.name = ""), (this.canuse = !1));
    const t =
        (null === (e = window.__global__) || void 0 === e
          ? void 0
          : e.__messager__) || window.parent.__global__.__messager__,
      ...
  }
};
```

`__global__` 的注入点 [inject/preload/electron/index.js:108000-109200](/home/msojocs/github/wechat-web-devtools-linux/resources/app/js/extensions/inject/preload/electron/index.js:107950)：

```js
const E = (() => {
  if (process.contextIsolated) {
    const { contextBridge: e } = require("electron");
    return (t, n) => { e.exposeInMainWorld(t, n) };
  }
  return (e, t) => { globalThis[e] = t };
})();
...
E("__global__", { ...c, __messager__: _ });   // ← 只有 preload 跑起来才执行
```

这个 preload 必须由 webview 的 preload 脚本触发；在 Skyline Wine 环境下整段没执行。

## 4. 路径是怎么生成的

### 4.1 wx devtools 侧拼路径

[80fe453c1ce7cbd1ad8861357cd87ff7.js](/home/msojocs/github/wechat-web-devtools-linux/resources/app/js/80fe453c1ce7cbd1ad8861357cd87ff7.js)：

```js
const t = require("path");
let s = t.resolve(__dirname, "../");        // 原生 Linux 绝对路径
exports.extensionsPath = t.join(s, "./js/extensions");
```

`__dirname` 是 wx devtools 自己的 Linux 路径，Node `path.resolve` 不会补 `Z:`。

[9ee5d191b506a7ae7cc6743ef0c44799.js](/home/msojocs/github/wechat-web-devtools-linux/resources/app/js/9ee5d191b506a7ae7cc6743ef0c44799.js)：

```js
this.setAttribute("webpreferences", r.join(",")),
this.setAttribute("preload",
  "file://" + e.join(t.default.extensionsPath,
    "inject/preload/electron/index.js"));
```

拼出 `file:///home/msojocs/...`。

### 4.2 传到 Skyline Server 侧

wx devtools 的 `preload.js`（`resources/app/js/electron/preload.js`）劫持 `createElement('webview')`，把 OUTER webview 的属性 setter 转发到 INNER（Skyline Server 的 webview），关键片段 [preload.js:112-117](/home/msojocs/github/wechat-web-devtools-linux/resources/app/js/electron/preload.js:112)：

```js
this.setAttribute = function (name, value) {
    if (name === 'webpreferences') {
        value += ',enableBlinkFeatures=SharedArrayBuffer'
    }
    return webview.setAttribute(name, value)
}
```

`webview` 是 `controller.webview`（Skyline Server 端的 Controller 实例），`setAttribute` 经 RPC `type: "dynamicProperty"` 转到 Skyline Server 的 [render-server.js:681-711](/home/msojocs/github/skyline-client-server/packages/electron/render-server.js:681)，由它直接赋给真实 webview 元素。**这条链路只是字符串透传，不做路径修正**。

## 5. 当前系统边界

```
原生 Linux 进程（wx devtools Electron）
  └─ renderer 进程（带 preload.js 拦截 createElement）
       └─ 拼出 preload 路径：file:///home/msojocs/...
            └─ 经 RPC 把属性转发到 Skyline Server

Wine 进程（Skyline Server Electron）
  └─ main-server.js → BrowserWindow (webviewTag: true)
       └─ <webview id="devtool" partition="trusted">      ← index.html 静态占位
       └─ <webview id="appservice" preload="file:///home/...">
            └─ appservice guest renderer
                 └─ 期望：electron 加载 preload → 注入 window.__global__
                 └─ 实际：路径在 Wine file:// handler 失败，preload 没跑
```

注意 Skyline Server 这边的 `Controller` 类（[render-server.js:176-242](/home/msojocs/github/skyline-client-server/packages/electron/render-server.js:176)）只创建 webview、不解析或修正路径——所有属性都是 wx devtools 侧发什么就收什么。

## 6. 修复方向

需要在路径**进入 Wine Electron 之前**补 `Z:` 盘符。三条可选路径（按改动量由小到大）：

### 6.1 在 Skyline Server 的 dynamicProperty handler 里修正（推荐）

[render-server.js:681-711](/home/msojocs/github/skyline-client-server/packages/electron/render-server.js:681) 收到 `action === "preload"` 时，对路径做 `file:///X → file:///Z:/X` 的转换，再 `instance.preload = params[0]`。改动最小，Skyline Server 自己最清楚自己跑在 Wine。

### 6.2 在 Skyline Server 的 webview 创建阶段统一加 hook

在 [main-server.js:781-794](/home/msojocs/github/skyline-client-server/packages/electron/main-server.js:781) 的 `app.whenReady()` 里加 `app.on("web-contents-created", ...)` 拦截器，对所有 webview 的 preload 做路径修正。覆盖更全，但会影响所有 webview。

### 6.3 改 wx devtools 端的路径生成

让 wx devtools 在拼 preload 路径时先检测运行环境（新增 "Skyline Server 接管" 标记），给 `extensionsPath` 补 `Z:` 前缀。问题是 wx devtools 跑在原生 Linux、并不是 Windows，需要 Skyline Server 通过 RPC 通知 wx devtools 当前运行模式。

### 6.4 顺带要补的事

只修路径让 preload 加载还不够——preload 里的 `__messager__: _`（`WebviewEnvMessagerService` 代理）需要 Skyline Server 提供 `setup / send / onMessage / invoke` 这套 RPC 通道。当前 Skyline Server 的 Controller 只暴露 `global.sendMessageSync` 给**主进程**用，没暴露给 webview guest——preload 能跑起来后会立刻在 `t.setup(...)` 处再次失败。修 preload 路径 + 补 guest→main RPC 通道，两件事必须一起做。

## 7. 复现步骤

1. 启动 wx devtools，打开 skyline 项目，让 simulator 启动 appservice。
2. 在 Skyline Server 上 `curl http://127.0.0.1:9222/json/list` 拿调试端点。
3. 连接 host page target，查询 `document.querySelector('#appservice').preload`，会看到 `file:///home/msojocs/.../.../inject/preload/electron/index.js`。
4. 在同 host renderer 里 `fetch(file:///home/msojocs/...)` → Failed to fetch；`fetch(file:///Z:/home/msojocs/...)` → 200。
5. 连接 appservice guest renderer (`50DAD...`)，查 `typeof window.__global__` → `"undefined"`，确认 preload 没生效。
6. 触发 appservice 业务代码，会抛 `Cannot read properties of undefined (reading '__messager__')`，定位在 `ide:///extensions/appservice/index.js:6620`。
