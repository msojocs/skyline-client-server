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
