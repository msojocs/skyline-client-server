# Skyline

这是[微信开发者工具](https://github.com/msojocs/wechat-web-devtools-linux)的Skyine通信项目。

## 使用说明

### 手动

1. 运行 `pnpm run electron:download:windows`，从微信开发者工具安装包提取 Windows Electron 运行时；需要 Node.js、curl 和 7-Zip（`7z` 命令）。
2. 下载 release 中的 `skyline-server-win32-x86_64.tar.gz` 并解压。
3. 准备好解压后的 `electron` 应用目录中的 `server.js`、`node_modules` 和原生 `.node` 模块，将该应用目录复制到 `cache/electron-win32-x64/resources/app`。
4. 运行 `cache/electron-win32-x64/electron.exe`（Linux 下通过 Wine 运行）。

在本仓库开发时，准备好 `packages/electron` 的补充文件后，运行 `pnpm run electron:start:wine` 即可启动；该命令会自动下载或更新运行时，并建立 `resources/app` 到 `packages/electron` 的链接。这个入口属于本项目的 Skyline 应用，不包含微信安装包中的资源；Docker 中也使用同样的入口布局。

Windows 运行时来源在 `config/config.json` 的 `electron` 中配置，目前使用微信开发者工具 `2.02.2608070`（内置 Electron 36.6.0）。下载采用 `.tmp` 文件续传并在校验成功后写入缓存，安装包保存在 `cache/wechat_devtools_2.02.2608070_x64.exe`。解压仅保留主程序（重命名为 `electron.exe`）、根目录 DLL、PAK、DAT、BIN 文件、Vulkan 配置（如有）和 `locales/`；不提取 `resources/`、安装器插件、CLI 和文件监视器。`resources.pak` 属于 Chromium 运行必需文件，会保留。

运行时缓存会记录下载来源；来源改变、必要文件缺失或仍为旧的官方 Electron 缓存时，会重新提取。Docker 镜像和 Windows CI 使用同一下载脚本与配置。

Electron 主进程会在 `9222` 端口开启远程调试；可通过 `REMOTE_DEBUGGING_PORT` 覆盖端口。

### Docker
> [!WARNING]
> Docker存在字体渲染问题，暂未解决。

```shell
docker run -d \
  --network host \
  -p 9222:9222 \
  -p 3001:3001 \
  -e HOST_UID=$(id -u) \
  -e HOST_GID=$(id -g) \
  --platform="linux/amd64" \
  --volume="/dev/shm:/dev/shm" \
  --name skyline_server \
  ghcr.io/msojocs/skyline-client-server:master
```
