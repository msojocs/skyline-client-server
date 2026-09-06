# Skyline

这是[微信开发者工具](https://github.com/msojocs/wechat-web-devtools-linux)的Skyine通信项目。

## 使用说明

### 手动
1. 安装 Electron 36.6.0（或使用发布包中附带的 Electron 运行时）。
2. 下载 release 中的 `skyline-server-win32-x86_64.tar.gz` 并解压。
3. 在解压后的 `electron` 目录中放入 `server.js`、`node_modules` 和原生 `.node` 模块。
4. 运行 `electron electron`（Windows 使用 `electron.exe electron`）。

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
