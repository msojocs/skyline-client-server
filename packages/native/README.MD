# Native 模块

native 模块使用 Rust 和 N-API 实现，共用同一套 RPC（`transport.rs` + `binding.rs`），
差异只在应用层——暴露的类/方法/字段不同：

| 产物 | 进程 | 应用层 |
| --- | --- | --- |
| `render-client.node` | 微信 devtools renderer | `Controller` / webview 元素（`client.rs`） |
| `render-server.node` | Skyline renderer | `render-server.ts` 的 `clazzMap` |
| `main-client.node` | 微信 devtools main 层 | `mainController.electron.*`（`main_client.rs`） |

`render-server.node` 本身不含应用层，复用为 Skyline main 进程的 RPC 服务端，
应用层是由 `main-rpc.ts` 构建并合入 `packages/electron/main-server.js`。

```shell
pnpm --filter nt-native compile            # 三个产物
pnpm --filter nt-native compile:main-client # 只出 main-client.node
pnpm --filter nt-native compile:windows
pnpm --filter nt-native test
```

`--module` 可取 `client` / `server` / `main-client` / `all`（默认 `all`，历史写法 `both` 等同）。

`compile:windows` 的目标是 `x86_64-pc-windows-gnu`，需要 Linux 上安装 `gcc-mingw-w64-x86-64`。Rust 的 `napi/dyn-symbols` 特性使模块直接从 Node/Electron 加载 N-API，不需要下载或链接 Electron 的 `node.lib`。

## 连接 main 层服务端

`mainController.connect(host = '127.0.0.1', port = 3002)` 返回 `Promise<void>`。
TCP 连接和服务端握手在后台线程中执行，等待期间不会阻塞 JavaScript 事件循环。
调用远端 Electron API 前需要等待连接完成：

```js
const { mainController } = require('./main-client.node')
await mainController.connect('127.0.0.1', 3002)
const webContents = mainController.electron.webContents.fromId(1)
```

参数错误、连接失败或握手失败都会 reject Promise。连接期间重复调用会等待同一次连接，
已连接时重复调用会成功返回并保留现有代理；`disconnect()` 会使尚未完成的连接失效。

## webview 挂载状态与消息发送

`controller.webview.parentElement` 是只读属性，返回远端父节点的对象代理；未挂载时，服务端的 `null`
按现有 RPC 规则解码为 `undefined`。
父节点通过句柄传输，不递归序列化 DOM，重复读取会复用同一代理。
服务端保留 `HTMLDivElement` 类型，由 Rust 层创建同名代理，目前只提供只读 `id` 属性。
属性、方法返回值和回调参数均使用这条类型映射，不依赖 `parentElement` 属性名。普通数据对象继续按值传输。

```js
const webview = controller.webview
controller.mount()
if (webview.parentElement) {
  webview.send('host___message__', { cmd: '23', data: { command: 'SAC0', data: options } })
}
```

`send(channel, ...args)` 转发到真实 Electron webview，远端调用失败会同步抛出。
这两个接口需要配套更新 `render-client.node` 和包含父节点句柄编码的 `render-server.js`。

webview 回调中的 `Event` 代理提供只读 `channel` 和 `args` 属性，读取时通过 RPC 获取远端事件字段。
`ipc-message` 监听器可用 `event.channel` 分发消息，再从 `event.args[0]` 读取消息内容；
`args` 中的数组和对象沿用现有 RPC 解码规则。这两个属性只需更新 `render-client.node`。

## executeJavaScript

native client 保留 Electron webview 的 Promise 语义：

```js
const result = await controller.webview.executeJavaScript('1 + 2')
// result === 3
```

参数编码失败、远端脚本执行失败、连接断开或 300 秒 RPC 超时都会 reject Promise。等待远端结果在 N-API async work 中进行，不阻塞 native client 的 JavaScript 事件循环，因此等待期间仍可处理 Controller prompt callback 和其它 RPC 消息。

## 一次性事件监听（main 层）

WebContents 代理支持 `once(eventName, listener)`，监听器在事件首次触发时执行，
事件参数通过 RPC 传回客户端。一次性监听由服务端 Electron 的 `once` 处理，
返回同一个 WebContents 代理，支持链式调用。

```js
const wc = mainController.electron.webContents.fromId(1)
console.info('fromId(1).id:', wc.id)
wc.once('destroyed', () => {
  console.info('webContents destroyed')
})
```

## 插件加载（main 层）

`mainController.electron.webContents.fromId(id)` 返回的 WebContents 代理上，`session` 是属性、
`session.extensions` 又是属性，两者都由 `main_client.rs` 的 `CLASSES` 表（`Session` / `Extensions`）
按远端 `instanceType` 复活成代理，所以属性链可以任意分层：

```js
const target = mainController.electron.webContents.fromId(0)
const extension = await target.session.extensions.loadExtension('/path/to/extension')
target.session.extensions.getAllExtensions()
```

main client 会在转发 `loadExtension` 前将路径参数的 `/` 前缀改为 `Z:/`，
例如 `/home/user/extension` → `Z:/home/user/extension`。
已有盘符的 file URL 和普通文件路径保持原样。

`loadExtension` 与 `executeJavaScript` 一样列在 `client.rs` 的 `ASYNC_METHODS` 里，返回 Promise：
加载失败、连接断开或 300 秒超时都表现为 reject（同步分支的 RPC 超时只有 5 秒，插件加载可能更久）。
其余 `Extensions` 方法（`getAllExtensions` / `getExtension` / `removeExtension`）是同步阻塞调用。
返回的 `Extension` 在 Electron 里是普通数据结构，走 JSON 直传，不需要额外的代理类。

## 请求拦截（main 层）

`session.webRequest` 返回可调用成员的对象。服务端返回 `WebRequest` 类型时使用
`main_client.rs` 的类表；Electron 36 的匿名原生对象会返回 `Object` 类型，客户端由
`client.rs` 的 `remote_object` 创建普通 Object 原型的代理，按需读取并递归解码属性。
函数成员绑定到原远端对象，支持按事件名动态取值，也支持取出后单独调用：

```js
const target = mainController.electron.webContents.fromId(0)
target.session.webRequest[t]({ urls: ['<all_urls>'] }, requestListernerFactory(t))
```

监听器是客户端函数，参数编码成 `{callbackId}`；服务端 `common/hook-argument.ts` 的 `hookArgument`
把它还原成真实函数再交给 Electron。监听器拿到的第二个参数是 Electron 的 callback，
服务端把它编成 `{instanceId, instanceType: 'function'}`（`functionData` 表），客户端解码成
可调用代理——所以下面这种回调式监听器可以跨进程工作，包括稍后再调用 callback：

```js
webRequest.onBeforeRequest({ urls: ['<all_urls>'] }, (details, callback) => {
  callback({ cancel: details.url.includes('blocked') })
})
```

同步模式下服务端用 `sendMessageSync` 等客户端把监听器跑完（回调在监听器内同步调用时走的是
嵌套 RPC），`__asyncCallback` 的函数则用 `sendMessageSingle` 发完就走。函数代理的调用结果
就是服务端那个函数的返回值，客户端 `callback(response)` 会把它带回来。
