# Skyline

这是[微信开发者工具](https://github.com/msojocs/wechat-web-devtools-linux)的Skyine通信项目。

## 使用说明

### 手动
1. 下载nwjs 0.54.1 win x64
2. 下载release中的skyline-server-win32-x86_64.tar.gz
3. 解压后将skyline-server-win32-x86_64中的文件复制到nwjs的根目录，改名package.nw
4. 使用wine运行nwjs中的nw.exe

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