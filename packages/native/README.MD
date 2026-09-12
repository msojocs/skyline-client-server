# Native 模块

native 模块使用 Rust 和 N-API 实现，共用同一套 RPC（`transport.rs` + `binding.rs`），
差异只在应用层——暴露的类/方法/字段不同：

| 产物 | 进程 | 应用层 |
| --- | --- | --- |
| `render-client.node` | 微信 devtools renderer | `Controller` / webview 元素（`client.rs`） |
| `render-server.node` | Skyline renderer | `server.ts` 的 `clazzMap` |
| `main-client.node` | 微信 devtools main 层 | `mainController.electron.*`（`main_client.rs`） |

`render-server.node` 本身不含应用层，复用为 Skyline main 进程的 RPC 服务端，
应用层是同目录的 `packages/electron/main-rpc.js`。

```shell
pnpm --filter nt-native compile            # 三个产物
pnpm --filter nt-native compile:main-client # 只出 main-client.node
pnpm --filter nt-native compile:windows
pnpm --filter nt-native test
```

`--module` 可取 `client` / `server` / `main-client` / `all`（默认 `all`，历史写法 `both` 等同）。

`compile:windows` 的目标是 `x86_64-pc-windows-gnu`，需要 Linux 上安装 `gcc-mingw-w64-x86-64`。Rust 的 `napi/dyn-symbols` 特性使模块直接从 Node/Electron 加载 N-API，不需要下载或链接 Electron 的 `node.lib`。

## executeJavaScript

native client 保留 Electron webview 的 Promise 语义：

```js
const result = await controller.webview.executeJavaScript('1 + 2')
// result === 3
```

参数编码失败、远端脚本执行失败、连接断开或 300 秒 RPC 超时都会 reject Promise。等待远端结果在 N-API async work 中进行，不阻塞 native client 的 JavaScript 事件循环，因此等待期间仍可处理 Controller prompt callback 和其它 RPC 消息。

## 插件加载（main 层）

`mainController.electron.webContents.fromId(id)` 返回的 WebContents 代理上，`session` 是属性、
`session.extensions` 又是属性，两者都由 `main_client.rs` 的 `CLASSES` 表（`Session` / `Extensions`）
按远端 `instanceType` 复活成代理，所以属性链可以任意分层：

```js
const target = mainController.electron.webContents.fromId(0)
const extension = await target.session.extensions.loadExtension('/path/to/extension')
target.session.extensions.getAllExtensions()
```

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

监听器是客户端函数，参数编码成 `{callbackId}`；服务端 `main-rpc.js` 的 `decodeArgument`
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
