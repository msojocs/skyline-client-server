# Skyline服务器启动文件

## 说明

此目录文件用于运行Skyline渲染服务器；

准备好 [补充文件](#补充文件) 后，使用wine调用 `nwjs-v0.54.1-x64` 启动。

## 补充文件

├─node_modules
│  ├─sharedMemory (重写版)
│  ├─skyline-server (编译后的SkylineServer native项目文件)
│  └─skyline-addon (官方原版)
└─webview
   └─skyline.js (编译后的ts项目文件) 