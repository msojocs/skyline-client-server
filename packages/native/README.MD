# Native 模块

native 模块使用 Rust 和 N-API 实现，客户端输出 `skyline.node`，服务端输出 `server.node`。

```shell
pnpm --filter nt-native compile
pnpm --filter nt-native compile:windows
pnpm --filter nt-native test
```

`compile:windows` 的目标是 `x86_64-pc-windows-gnu`，需要 Linux 上安装 `gcc-mingw-w64-x86-64`。Rust 的 `napi/dyn-symbols` 特性使模块直接从 Node/Electron 加载 N-API，不需要下载或链接 Electron 的 `node.lib`。
