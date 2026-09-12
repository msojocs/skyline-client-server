# AppService Prompt 同步返回架构报告

## 1. 结论

微信开发者工具的 appservice 在 Electron 中把 window.prompt 改写成同步 IPC：

    ipcRenderer.sendSync("prompt", message)

因此 Skyline Electron 主进程最终必须设置 `event.returnValue`，否则 guest renderer 会永久等待。不过，Electron 36.6 允许主进程保留该 event，在 UI 或 Controller 异步完成后再写入 `returnValue`。被阻塞的只有发起 `sendSync` 的 guest renderer，主进程和宿主 renderer 仍能处理 IPC、WebSocket 和用户操作。

Controller 可以参与两类方案：

    固定或可预取数据：提前缓存，prompt 时立即返回
    需要交互的数据：主进程保留 event，Controller 异步处理，resolveDialog 后返回

关键限制不是“必须在监听器返回前设置”，而是不能阻塞 Electron 主线程。以下写法仍然不安全：

    Atomics.wait(...)
    child_process.spawnSync(...)
    ipcMain.on('prompt', async () => await 同一主线程才能完成的操作)

推荐优先级：

1. 在 appservice URL 中携带真实的 cts 参数。
2. 由 Controller callback 按请求返回真实 compileTs，再调用 `resolveDialog`。
3. 由 Controller 或微信 renderer 提前发布 compileTs，Skyline main 缓存后立即返回。
4. 无处理器、超时或连接断开时返回明确错误，绝不伪造 compileTs=0。

## 2. 当前系统边界

当前系统至少有以下进程和对象：

    微信开发者工具 Electron
    ├─ 微信 renderer
    │  ├─ simulator 状态
    │  ├─ compileCommand.ts
    │  └─ 原版 Webview/dialog handler
    │
    └─ Skyline Electron
       ├─ main.js
       │  └─ ipcMain.on("prompt")
       └─ Skyline Server renderer
          ├─ server.js
          ├─ TypeScript Controller
          ├─ skyline-server/render-server.node
          └─ <webview id="appservice">
             └─ appservice guest renderer
                └─ 微信 preload

本项目的 Controller 位于 Skyline Server renderer：

[controller.ts](/home/msojocs/github/skyline-client-server/packages/typescript/src/server/controller.ts:3)

当前 Controller 负责：

    创建 webview
    暴露 webview 属性
    mount()
    unmount()
    setDialogCallback(callback)
    dialog(webview, request)
    resolveDialog(requestId, result)

它不持有微信开发者工具的 simulator store，也不天然知道 compileCommand.ts。

当前 Electron 主进程入口：

[main.js](/home/msojocs/github/skyline-client-server/packages/electron/main.js:25)

当前 native client 对 Controller 的暴露：

[client.rs](/home/msojocs/github/skyline-client-server/packages/native/src/client.rs:18)

    Controller:
      methods: mount, unmount, setDialogCallback, dialog, resolveDialog
      property: webview

## 3. prompt 的实际链路

微信 preload 会把浏览器原生对话框改成同步 IPC：

    window.prompt = (...args) => ipcRenderer.sendSync("dialog", "prompt", ...args)
    window.alert = (...args) => ipcRenderer.sendSync("dialog", "alert", ...args)
    window.confirm = (...args) => ipcRenderer.sendSync("dialog", "confirm", ...args)

appservice 首次加载时的链路：

    appservice guest
      -> window.prompt("GET_RUNTIME_INSTANCE_INFO")
      -> preload hackElectronDialog
      -> ipcRenderer.sendSync("prompt", ...)
      -> Skyline Electron main.js
      -> event.returnValue
      -> guest prompt() 返回

如果 main.js 没有设置 event.returnValue，guest renderer 会永久等待。此时：

- setTimeout 不会执行；
- dom-ready 不会触发；
- did-finish-load 不会触发；
- DevTools Runtime.evaluate 可能超时；
- HTML 网络请求虽然可能已经返回 200，但文档执行仍然卡住。

## 4. GET_RUNTIME_INSTANCE_INFO 契约

### 4.1 请求

主 frame 请求：

    GET_RUNTIME_INSTANCE_INFO

instance frame 转发时可能是：

    $${"msg":"GET_RUNTIME_INSTANCE_INFO","common":{"compileTs":1788762045613}}

解析后的结构：

    {
      msg: "GET_RUNTIME_INSTANCE_INFO",
      common: {
        compileTs: 1788762045613
      }
    }

msg 是真实 prompt 内容，common.compileTs 是发送方 frame 所属的编译世代。

### 4.2 响应

返回值必须是 JSON 字符串：

    event.returnValue = JSON.stringify({
      compileTs: 1788762045613
    })

guest 随后执行：

    const result = prompt("GET_RUNTIME_INSTANCE_INFO")
    this.runtimeInstanceInfo = JSON.parse(result)

合法响应：

    {"compileTs":1788762045613}

不能返回 JavaScript 对象。不能返回空字符串、普通错误文本、空对象或 null 作为成功结果。

### 4.3 compileTs 的来源

微信 simulator 的状态初始值类似：

    compileCommand: {
      ts: 0,
      hrts: 0
    }

每次 SIMULATOR_COMPILE 会执行近似逻辑：

    compileCommand = {
      ...data,
      ts: data.ts || Date.now(),
      hrts: data.hrts || performance.now()
    }

所以：

    compileCommand.ts  是 wall-clock 毫秒时间戳，用作编译世代号
    compileCommand.hrts 是 performance.now()，只用于性能统计

GET_RUNTIME_INSTANCE_INFO 只需要 compileCommand.ts，不需要 hrts。

## 5. 其它 prompt、alert 和 confirm 类型

本节分析 GET_RUNTIME_INSTANCE_INFO 之外，微信 appservice、instance frame、pageframe 和通用 preload 实际使用的同步消息。需要区分两件事：

1. 返回值本身是什么；
2. 原版 handler 在返回值之外是否还产生状态更新、事件转发或用户界面副作用。

仅返回空字符串可以解除 sendSync，但不能自动复现这些副作用。

### 5.1 prompt: CONTINUE_LOAD

调用位置是 appservice 的 subLoader，在主包脚本准备加载时：

    if (__global.prompt("CHECK_SUBPACKAGE_READY___APP__") !== "yes") {
      wait for SUBPACKAGE_READY___APP__
      __global.prompt("CONTINUE_LOAD")
    }

原版固定返回：

    "yes"

语义是允许等待完成后的脚本继续注入。这个请求本身不依赖项目数据，也没有额外副作用，因此可以直接返回 "yes"。

它通常只会在前一个 CHECK_SUBPACKAGE_READY___APP__ 返回非 "yes" 时出现。如果前一个请求直接返回 "yes"，CONTINUE_LOAD 不会发生。

### 5.2 prompt: CHECK_SUBPACKAGE_READY___APP__

这个请求不是固定值。原版判断主包是否已经加载：

    sharedManager.isPackageLoaded(
      MINI_PROGRAM_MAIN_PACKAGE_ROOT
    )
      ? "yes"
      : "no"

调用方行为：

| 返回值 | 调用方行为 |
| --- | --- |
| "yes" | 立即继续加载主包脚本 |
| "no" | 等待 SUBPACKAGE_READY___APP__ 事件，再调用 CONTINUE_LOAD |

这是当前 fallback 最容易产生隐藏死锁的请求。若返回错误字符串，调用方看到的只是“不是 yes”，因此会进入等待分支；如果没有另外的 SUBPACKAGE_READY___APP__ 事件，appservice 会永久等待。

所以该请求不能简单按未知 prompt 返回错误字符串。可选策略：

1. 兼容优先：直接返回 "yes"，跳过等待。这不需要外部状态，但假设主包资源已经可用。
2. 状态准确：从实际编译/加载状态获取并返回 "yes" 或 "no"。
3. 失败优先：返回错误前必须同时让调用方抛错或结束等待；仅返回错误字符串是不够的，因为当前调用方没有错误协议。

在不实现主包状态同步的最小方案中，建议返回 "yes"，而不是返回错误字符串。

### 5.3 prompt: GET_USER_DATA_PATH

原版返回 IAppService.getDataPathDefault()。

这是一个本地文件系统路径字符串，不是 JSON，也不是 URL。它依赖微信开发者工具的用户数据目录；Skyline Electron 自己的 app.getPath("userData") 不一定等价。

如果调用方将返回值当作路径使用，返回“暂不支持”错误字符串不会立即在 prompt 处失败，而可能在稍后的文件操作中表现为路径不存在。因此：

- 不能用空字符串表示成功；
- 不能用 Skyline 自己的 userData 路径冒充微信工具路径；
- 如果没有可靠的微信路径来源，应返回明确错误并记录请求；
- 如果只是为了让无关页面继续运行，可以返回 Skyline 路径，但这属于兼容性降级，必须明确标记。

### 5.4 prompt: GET_MESSAGE_TOKEN

这个请求由通用 EnvMessagerService 使用，不只属于 appservice。调用逻辑是：

    token = prompt("GET_MESSAGE_TOKEN")
    if (token) return token
    if (window.__global.messageToken) return window.__global.messageToken
    return tokenFromUserAgent("envWSSToken")

因此空字符串有明确语义：

    空字符串 -> 继续使用 global.messageToken 或 UA 中的 envWSSToken

返回错误字符串反而会被当成有效 token，导致后续 WebSocket 使用错误凭证。

但空字符串不等于 WebSocket 可用。如果 global 和 UA 中都没有 token，最终仍然无法连接微信环境 WebSocket。当前 Skyline appservice UA 没有 envWSSPort/envWSSToken，因此这里只能表示“没有 token，继续 fallback”。

### 5.5 普通 prompt

例如用户代码或某个 webview 调用：

    prompt("请输入内容")

原版 appservice/pageframe handler 对未识别 prompt 通常直接 dialog.ok("")。它不会自动显示浏览器输入框。其它专门的 webview 组件可能把 prompt 转换为开发者工具自己的确认 UI，但那是组件级 handler，不是全局 IPC 默认行为。

如果没有实现用户交互 UI，不能声称支持普通 prompt。实现上可以选择：

    未识别 prompt -> 明确错误

或保持原版 appservice 行为：

    未识别 prompt -> ""

前者便于定位问题，但调用方可能把错误文本当成用户输入；后者可继续执行，但无法获得用户输入。

## 6. alert 类型及其副作用

### 6.1 alert: MAINFRAME_LOADED

appservice 主 frame 的初始化代码在文档加载完成后发送：

    alert("MAINFRAME_LOADED")

原版处理是确认 dialog 并阻止默认处理。它主要告诉宿主 mainframe 已经执行到加载完成位置。当前 main.js 返回空字符串可以解除同步调用，但不会向微信开发者工具 renderer 发送 mainframe loaded 状态。

### 6.2 alert: DOCUMENT_READY

这个消息的副作用比 MAINFRAME_LOADED 更重要。不同组件的原版行为包括：

- appservice loader：确认 dialog，将 appservice 标记为 ready，并触发 app launched；
- pageframe：确认 dialog，调用 loadPage()，设置 documentReady；
- agent：确认 dialog，结束 agent loading 状态并同步主题。

因此 alert("DOCUMENT_READY") 的返回值虽然是空字符串，但不能理解成“什么都不做”。如果页面完全由 Skyline 管理，空字符串可以作为降级确认；如果还要保持原版调试器状态，必须转发事件。

### 6.3 alert: USER_CODE_READY

pageframe 在用户代码注入完成后发送：

    alert("USER_CODE_READY")

原版 pageframe handler 会调用 onPageUserCodeReady()，进而设置 RUNTIME_STEP_3_PAGE_READY。这不是 appservice mainframe 的初始握手，而是普通页面 frame 的“用户代码已执行”通知。对当前 appservice mainframe 通常不是必需请求，但如果同一 IPC handler 被 pageframe webview 使用，只返回空字符串会导致外部页面一直处于未 ready 状态。

### 6.4 alert: SET_SOCKET_HEADER:<JSON>

网络模块会发送：

    SET_SOCKET_HEADER:<serialized-json>

例如：

    SET_SOCKET_HEADER:{"Origin":"https://example.com"}

原版行为：

1. 调用 dialog.ok("") 解除同步 alert；
2. 去掉 SET_SOCKET_HEADER: 前缀；
3. 对剩余字符串执行 JSON.parse；
4. 将解析结果保存为后续 WebSocket 请求使用的 header；
5. JSON 解析失败时清空 header。

payload 是 JSON 对象字符串，可能包含 Origin、Cookie 或 Authorization。当前 main.js 只返回空字符串并不会保存这些 header，后续用户 WebSocket 连接可能因此缺少认证信息。

### 6.5 alert: GET_WEBVIEW_SCROLL_Y<value>

pageframe 分享截图时会执行类似代码：

    __global.alert("GET_WEBVIEW_SCROLL_Y" + window.scrollY)
    window.scrollTo(0, 0)

原版 handler 确认 alert、提取数值、保存原始 scrollY、截图，截图完成后恢复 scrollY。只返回空字符串可以解除同步 alert，但不能恢复用户页面滚动位置。这属于 pageframe 功能，不是 appservice 启动必需功能。

### 6.6 alert: contextmenu:<x>:<y>

documentstart 会在右键菜单事件中发送：

    alert("contextmenu:" + clientX + ":" + clientX)

原版不同 webview handler 可能根据坐标打开开发者工具菜单。当前返回空字符串的效果是右键事件被确认，但不会打开宿主 context menu。它不会导致加载阻塞，但会丢失调试器菜单功能。

### 6.7 alert: 进入客服会话

部分 pageframe 逻辑会把“进入客服会话”作为 alert 文本交给宿主，原版显示确认 UI，用户确认后才调用 dialog.ok。直接返回空字符串会跳过确认 UI，改变用户交互语义，但不会导致 guest 死锁。

## 7. confirm 类型

### 7.1 原版返回行为

原版全局 Electron fallback 在没有专门 handler 时设置：

    event.returnValue = ""

由于 preload 直接返回这个值：

    window.confirm(...) -> ""

空字符串是 falsey，因此等价于用户选择取消。部分 webview 组件会显示自己的确认 UI，并根据用户操作调用 dialog.ok() 或 dialog.cancel()；这是组件层功能，不是全局 IPC 默认逻辑。

### 7.2 当前最小策略

当前 main.js 对 confirm 返回空字符串，语义是所有 confirm 默认取消。

优点：

- 不阻塞；
- 与原版无专门 handler 时的 falsey fallback 一致；
- 不需要异步用户交互。

缺点：

- 不显示确认 UI；
- 所有需要用户确认的操作都会自动走取消分支；
- 调用方得到的是 string，而不是 boolean false。

不能返回错误字符串，因为非空字符串在 JavaScript 中是 truthy，会把“暂不支持”误判成用户点击了确定。

### 7.3 confirm 的返回类型

如果只兼容原版 preload 的直接行为，返回空字符串最接近原实现。如果能够确认调用方要求标准 boolean，可以返回 false，但这是相对于原版字符串 fallback 的行为变化。

## 8. 双美元包装消息和处理顺序

instance frame 的 alert/prompt 可能被包装为：

    双美元前缀 + {"msg":"DOCUMENT_READY","common":{"compileTs":1788762045613}}

处理顺序必须是：

1. 去掉双美元前缀；
2. 解析 JSON；
3. 读取 msg 作为真正请求；
4. 读取 common.compileTs 做编译世代校验；
5. 决定返回值或丢弃旧消息。

不能只根据原始字符串做固定匹配，否则 instance frame 的消息会落入未知分支。

对需要特定返回值的请求，必须返回精确的 "yes"、空字符串或 JSON 字符串。错误字符串可能被调用方当成普通业务结果。

## 9. 当前最小返回策略评估

当前 main.js 的策略：

    CONTINUE_LOAD -> "yes"
    GET_MESSAGE_TOKEN -> ""
    contextmenu/nwmenu -> ""
    未识别 prompt -> 错误字符串
    alert -> ""
    confirm -> ""

逐项评价：

| 请求 | 当前策略 | 评价 |
| --- | --- | --- |
| CONTINUE_LOAD | "yes" | 正确，固定确认 |
| CHECK_SUBPACKAGE_READY___APP__ | 错误字符串 | 有风险，会被调用方当成非 yes 并进入无限等待 |
| GET_MESSAGE_TOKEN | "" | fallback 语义正确，但不提供实际 token |
| GET_USER_DATA_PATH | 错误字符串 | 不伪造路径是正确的，但使用方可能把错误当路径 |
| 普通 prompt | 错误字符串 | 能暴露不支持，但调用方可能把错误当用户输入 |
| MAINFRAME_LOADED | "" | 能解除同步调用，缺少宿主状态副作用 |
| DOCUMENT_READY | "" | 能解除同步调用，缺少 ready/app launch/pageframe 副作用 |
| USER_CODE_READY | "" | 对 pageframe 会缺少 ready 状态更新 |
| SET_SOCKET_HEADER | "" | 能解除同步调用，但不会保存 WebSocket header |
| GET_WEBVIEW_SCROLL_Y | "" | 能解除同步调用，但截图后无法恢复滚动位置 |
| contextmenu | "" | 能解除同步调用，但丢失菜单行为 |
| confirm | "" | falsey，接近原版 fallback；不显示 UI |

在不实现整体 dialog 事件转发的前提下，最需要调整的是：

    CHECK_SUBPACKAGE_READY___APP__ -> "yes"

否则当前“返回错误字符串以快速失败”的原则在这个请求上不成立，因为 appservice 调用方没有错误分支。

## 10. compileTs 的传播和作用

ASLoader 会先把响应保存为：

    this.runtimeInstanceInfo.compileTs

创建 instance frame 时复制：

    instanceInfo: {
      ...this.runtimeInstanceInfo
    }

frame 加载完成后会设置：

    WeixinJSBridge.__setCommonPayload(
      "compileTs",
      frame.instanceInfo.compileTs
    )

instance frame 转发 alert 或 prompt 时，把请求包装成：

    {
      msg: "...",
      common: {
        compileTs: frame.instanceInfo.compileTs
      }
    }

微信渲染层会将 common.compileTs 与当前 compileCommand.ts 做严格比较：

    payload.common.compileTs !== currentCompileCommand.ts

如果不相等，旧 frame 的消息会被忽略。这个检查用来防止：

- 旧 frame 触发新的 DOCUMENT_READY；
- 旧 frame 修改当前加载状态；
- 旧页面事件混入新页面；
- 编译重启后新旧页面互相覆盖；
- 重复导航产生 ERR_ABORTED (-3)。

因此 compileTs 不是普通参数，而是 frame 的世代隔离标识。

## 11. 不同返回值的后果

### 正确值

    {"compileTs":1788762045613}

JSON.parse 成功，且后续 frame 消息可以匹配当前编译世代。

### 空字符串或普通错误文本

    ""
    Skyline runtime info is not ready

JSON.parse 会立即抛错。它们适合 fail-fast，不适合表示成功。

### 空对象

    {}

JSON.parse 成功，但 compileTs 是 undefined。后续严格比较会失败，frame 消息会被当成旧消息。

### 字符串数字

    {"compileTs":"1788762045613"}

JSON.parse 成功，但 string 与 number 严格比较不相等。

### 0

    {"compileTs":0}

只有 simulator 尚未发生任何编译且当前 compileCommand.ts 也为 0 时才有意义。正常编译后当前值通常来自 Date.now()，固定 0 会造成世代不匹配。

## 12. 如何在 prompt 中安全调用 Controller

如果 Controller 位于 Skyline renderer，而 prompt 在 Skyline main 接收，链路会变成：

    guest sendSync('prompt')
      -> Skyline main ipcMain handler
         -> IPC 发送给 Skyline renderer
            -> Controller.dialog()
               -> IPC 回传 main
                  -> event.returnValue

这条链路可以工作，前提是主进程只保存 event 和转发请求，不阻塞主线程：

    ipcMain.on('prompt', (event, message) => {
      const requestId = createRequestId()
      pendingDialogs.set(requestId, event)
      event.sender.hostWebContents.send('skyline-dialog-request', {
        requestId,
        message,
      })
      // 不在这里设置 returnValue，guest 保持阻塞
    })

    ipcMain.on('skyline-dialog-response', (_, response) => {
      const event = pendingDialogs.get(response.requestId)
      event.returnValue = response.result
      pendingDialogs.delete(response.requestId)
    })

不应使用以下写法阻塞主线程：

    ipcMain.on('prompt', async (event, message) => {
      const result = await requestController(message)
      event.returnValue = result
    })

这里的问题不是 `async` 关键字本身，而是等待关系不透明，且很容易等待一个必须由当前事件循环处理的响应。明确的 request/resolve 协议更容易做来源校验、超时和重复完成保护。

Controller callback 必须用异步 RPC 消息投递，不能让 server 同步等待 callback 的返回。否则 callback 内再调用 `resolveDialog` 会形成嵌套同步 RPC 死锁。

## 13. 可选优化：预取、缓存、同步返回

### 13.1 总体流程

    编译或启动 appservice 前
      |
      +-- 微信 renderer 获取 compileCommand.ts
      |
      +-- Controller.setRuntimeInfo({ compileTs })
      |       或内部 WebSocket / IPC 推送
      |
      +-- Skyline main 保存 runtimeInfoCache
      |
      +-- 设置 webview.src
      |
      +-- guest 调用 prompt('GET_RUNTIME_INSTANCE_INFO')
      |
      +-- Skyline main 同步读取 runtimeInfoCache
              event.returnValue = JSON.stringify(runtimeInfoCache)

### 13.2 缓存结构

缓存不能只使用全局单例，至少应包含 session 和 webview 维度：

    {
      "s0": {
        "compileTs": 1788762045613,
        "receivedAt": 1788762045700,
        "source": "controller",
        "sessionId": "simulator-app-session-s0",
        "valid": true
      }
    }

推荐 scope：

    sessionId + appIdentity

最低限度使用：

    sid / winId / sessionId

不能把不同项目、窗口或编译世代共用一个 compileTs。

### 13.3 缓存状态

    EMPTY
      | 收到有效 runtime info
      v
    READY
      | 新编译世代到达
      v
    READY(new compileTs)
      | session 销毁、连接关闭或超时
      v
    STALE / EMPTY

命中 READY 且 scope 匹配的缓存时可以立即返回 JSON。没有缓存时可进入第 12 节的 deferred Controller 流程；没有注册处理器或处理超时则返回明确错误，不能伪造 compileTs=0。

## 14. Controller API 建议

### 14.1 setDialogCallback 的语义

以下接口用于“guest 同步等待、Controller 异步处理”：

    setDialogCallback(callback)
    dialog(webview, request)

callback 以 `(webview, requestId, type, ...args)` 接收参数，不再二次封装 dialog 参数。处理方最终调用 `resolveDialog(requestId, result)`。callback 使用 one-way RPC 投递，因此可以在 callback 内同步调用 `resolveDialog`，不会形成对端等待环。

### 14.2 已实现接口

    type DialogRequest = {
      requestId: string
      type: 'prompt' | 'alert' | 'confirm'
      args: unknown[]
      guestWebContentsId?: number
    }

    class Controller {
      setDialogCallback(callback): void
      dialog(webview, request): boolean
      resolveDialog(requestId, result?): void
    }

`dialog` 的布尔返回值只表示是否注册了 callback。实际 dialog 结果通过 `resolveDialog` 返回。

### 14.3 dialog(webview, request) 的处理

renderer 根据 `guestWebContentsId` 找到真实 `<webview>`，调用 `controller.dialog(webview, request)`。Controller 将 `(webview, requestId, type, ...args)` 异步投递给微信开发者工具注册的 callback。用户操作结束后，callback 调用 `controller.resolveDialog(requestId, result)`。

Controller callback 是进程级共享状态，因为 native client 新建的 Controller 实例与 renderer 启动时的 `global.controller` 不是同一个对象；dialog 路由必须能看到 native client 注册的处理器。

## 15. WebSocket 作为预取通道

### 15.1 微信内部 WebSocket 的特征

微信 MainWebSocketServerService：

- 端口随机分配；
- token 运行时生成；
- token 与允许的 mainProtocol 绑定；
- WebSocket 子协议携带 winId、主协议、子协议和 token；
- 消息使用内部 message-center envelope；
- 不会自动实现 GET_RUNTIME_INSTANCE_INFO handler。

协议形态近似：

    [winId|]MAIN_PROTOCOL[_subProtocol]#token

消息形态近似：

    {
      "channel": "__invoke__",
      "message": {
        "from": "...",
        "to": "...",
        "cmd": "...",
        "callbackId": "...",
        "data": {}
      }
    }

当前 Skyline appservice 的 UA 没有 envWSSPort 和 envWSSToken，所以它没有加入微信内部环境 WebSocket。项目自己的 3001 是 Skyline RPC，不是微信 message-center WebSocket。

### 15.2 必须增加微信侧 handler

即使 Skyline main 获得微信内部 WebSocket 的 port/token，也必须在微信 renderer 注册自定义请求，例如：

    SKYLINE_GET_RUNTIME_INFO

handler 返回：

    {
      compileTs: store.getState().simulator.compileCommand.ts
    }

WebSocket 只负责消息路由，不会自动读取 compileCommand.ts。

### 15.3 两种正确时序

预取路径：

    1. 微信 renderer 完成编译状态更新
    2. runtime-info handler 可以读取新的 compileCommand.ts
    3. Skyline 请求 SKYLINE_GET_RUNTIME_INFO
    4. Skyline main 更新缓存
    5. Skyline 创建或导航 appservice webview
    6. guest 调用 GET_RUNTIME_INSTANCE_INFO
    7. main 从缓存同步返回

deferred 路径：

    1. guest 已经调用 sendSync('prompt')
    2. Skyline main 保存 event 并发送异步请求
    3. Controller 或 WebSocket 返回 compileTs
    4. renderer 调用 resolveDialog
    5. main 设置 event.returnValue

第二条路径中 main 不能使用 `Atomics.wait`、同步子进程或其它方式阻塞事件循环。它只登记 pending request；真正被阻塞的是 guest renderer。

## 16. URL ?cts= 方案

appservice loader 会优先读取 URL 查询参数：

    ?cts=<compileTs>

例如：

    http://127.0.0.1:32594/appservice/s0/_sessionId/simulator-app-session-s0/mainframe?cts=1788762045613

这样 loader 会直接设置：

    runtimeInstanceInfo = {
      compileTs: Number(query.cts)
    }

从而跳过：

    prompt('GET_RUNTIME_INSTANCE_INFO')

优点：

- 不需要修改 preload；
- 不需要 prompt 时跨进程通信；
- 不需要 JS callback 穿过 IPC；
- 与 appservice loader 原生逻辑一致；
- 失败路径更容易观察。

缺点：

- 导航前必须取得真实 compileTs；
- 每轮编译必须更新 URL；
- 代理路由必须保留 query 参数；
- 不能复用旧 URL 或固定 0。

## 17. 失败和安全策略

### 17.1 没有处理器或数据

不要返回：

    {"compileTs":0}

应返回：

    Skyline Electron host does not support the "prompt" IPC yet: GET_RUNTIME_INSTANCE_INFO

这样 guest 会在 JSON.parse 处快速失败，主进程日志可以定位原因。若使用缓存优化，也可以返回包含 session/scope 的 “runtime info is not ready” 错误。

### 17.2 scope 不匹配

如果缓存属于 s0，但当前 prompt 来自 s1，应拒绝：

    Runtime info scope mismatch: expected s1, cached s0

### 17.3 compileTs 过期

缓存至少记录：

    cachedCompileTs
    receivedAt
    currentSessionId
    source

session 销毁或 WebSocket 断开后，应把缓存标记为 STALE。

### 17.4 WebSocket token

WebSocket token 应：

- 不写入普通日志；
- 不放进错误字符串；
- 不复用普通 HTTP UA token；
- 只允许目标协议；
- 连接断开后清理或标记缓存。

## 18. 已实现流程

### 阶段一：原始参数转发

preload 将 `type` 和 dialog 的原始参数直接发送给 main。main 不解析 `$$` 消息、不识别固定消息，也不构造协议兜底结果。

### 阶段二：动态请求进入 pendingDialogs

`prompt`、`alert`、`confirm` 的动态请求保存：

    {
      "requestId": "dialog-...",
      "type": "prompt",
      "args": ["GET_RUNTIME_INSTANCE_INFO"],
      "guestWebContentsId": 123,
      "hostWebContentsId": 45,
      "event": "原始 IpcMainEvent"
    }

main 随后通过 `skyline-dialog-request` 通知宿主 renderer，但不等待任何 Promise。

### 阶段三：Controller 异步转发

renderer 将请求交给 Controller：

    controller.dialog(webview, request)
      -> one-way native RPC callback
      -> 微信开发者工具处理请求或显示 UI

没有注册 callback 时，renderer 立即回复明确的“不支持”错误。

### 阶段四：resolveDialog 完成请求

处理完成后：

    controller.resolveDialog(requestId, result)
      -> renderer 发送 skyline-dialog-response
      -> main 校验响应来源
      -> main 设置原始 event.returnValue
      -> guest sendSync 返回并继续执行

重复 resolve 会被忽略。默认 30 秒超时可由 `SKYLINE_DIALOG_TIMEOUT_MS` 调整；超时会清理 pending 请求并解除 guest 阻塞。

### 阶段五：可选增加 runtime info cache

如果 `compileTs` 在导航前已经可用，可以增加 main 侧缓存，让 `GET_RUNTIME_INSTANCE_INFO` 直接返回。客服会话和普通交互 dialog 仍必须保留 request/resolve 流程。

## 19. 验收标准

### 正常启动

    mainframe 加载
      -> Controller 收到 GET_RUNTIME_INSTANCE_INFO
      -> resolveDialog 返回合法 JSON
      -> JSON.parse 成功
      -> dom-ready / did-finish-load 触发

### 连续编译

    compileTs = A
    启动页面 A
    compileTs = B
    启动页面 B

必须保证：

- 页面 B 不接收 A 的 $$ 消息；
- B 的 prompt 返回 B；
- A 的旧 frame 不会触发当前页面的 DOCUMENT_READY。

### 无处理器或处理超时

必须保证：

- 不永久卡住；
- 最终一定设置 event.returnValue；
- 不伪造业务结果；
- guest 按原始返回值处理失败；
- 主进程日志包含原始 prompt 内容。

### WebSocket 重连

必须验证：

- token 失效后不复用旧 token；
- 连接断开后缓存变为 stale；
- 新编译世代可以覆盖旧缓存；
- WebSocket 延迟不会阻塞 prompt handler。

## 20. 最终决策

采用统一协议：

    preload：发送 type 和原始 args
    main：保存 pending event 并直接转发
    Controller callback：调用微信原版 dialog handler
    resolveDialog：直接回传 ok/cancel 的结果

不建议：

    ipcMain.on('prompt', async (...) => await websocketRequest(...))

最终模型：

    appservice guest = 同步等待结果
    Skyline main = 保存 event、路由请求、验证响应、延迟回写
    Skyline renderer = webContentsId 到 webview 的映射
    Controller callback = 微信开发者工具异步处理或显示 UI
    resolveDialog = 完成原始同步调用

该模型已经在 Electron 36.6 中端到端验证：`GET_RUNTIME_INSTANCE_INFO` 返回后继续执行，后续 `alert` 延迟处理期间 guest 保持阻塞，resolve 后 guest 再继续执行下一个 prompt。
