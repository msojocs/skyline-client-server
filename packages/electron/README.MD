# Skyline服务器启动文件

测试版

## 说明

此目录文件用于运行 Skyline 渲染服务器的 Electron 应用；

准备好 [补充文件](#补充文件) 后，运行 Electron 的 `main-server.js`。

## 补充文件

├─node_modules
│  ├─sharedMemory (重写版)
│  ├─skyline-server (编译后的SkylineServer native项目文件)
│  └─skyline-addon (官方原版)
├─render-server.js (编译后的ts项目文件)
├─main-server.js (Electron main 进程与 main 层 RPC 服务端的合并产物)
└─...

Electron 使用 `main-server.js` 创建窗口并提供 main 层 RPC；渲染层兼容脚本提供项目原先使用的 NW.js webview 方法名。
