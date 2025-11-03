# Skyline服务器启动文件

测试版

## 说明

此目录文件用于运行Skyline渲染服务器；

准备好 [补充文件](#补充文件) 后，使用wine调用 `node.exe` 启动。

## 补充文件
```
├─node_modules
│  ├─sharedMemory   (重写版共享内存模块)
│  ├─skyline-server (编译后的SkylineServer native项目文件)
│  └─skyline-addon  (官方原版)
├─node.dll          (伪造的node.dll库，实际转发给node.exe)
├─node.exe          (开起了 `COMPRESS_POINTERS` 的node.exe，版本v16.4.0，到skyline-node下载或者自己编译)
├─nw.dll            (伪造的nw库，实际转发给node.exe)
├─skyline.js        (编译后的ts项目文件)
├─ucrtbased.dll     (运行时，放在wine的目录也行)
└─vcruntime140d.dll (运行时，放在wine的目录也行)
```

[skyline-node](https://github.com/msojocs/skyline-node)

## 调用
```
skyline.node
   │ 
   ├─nw.dll
   │ 
   └─node.dll
```