# Skyline

这是微信开发者工具的Skyine通信项目。

## 使用说明

### 自动
```shell
docker run -it \
  --rm \
  --hostname="$(hostname)" \
  -e USER_NAME="docker" \
  -e USER_UID="$(id -u)" \
  -e USER_GID="$(id -g)" \
  --env="DISPLAY" \
  --platform="linux/amd64" \
  --volume="${XAUTHORITY:-${HOME}/.Xauthority}:/root/.Xauthority:ro" \
  --volume="/tmp/.X11-unix:/tmp/.X11-unix:ro" \
  --volume="/dev/shm:/dev/shm" \
  -p 3001:3001 \
  ghcr.io/msojocs/skyline-client-server:master
```

### 手动
1. 编译全部项目，client.node/server.node/server.js/nw.dll/node.dll
4. 复制`node.dll` `nw.dll` `server.js`，放到`packages/nodejs`
4. 复制`ucrtbased.dll` `vcruntime140d.dll`，放到`packages/nodejs`
3. 下载[skyline-node](https://github.com/msojocs/skyline-node/releases/tag/v16.4.0-1)放到`packages/nodejs`
4. 复制windows的`skyline-addon`，放到`packages/nodejs/node_modules`
5. 复制`skylineServer.node`，放到`packages/nodejs/node_modules/skyline-server`
6. 复制`sharedMemory.node`，放到`packages/nodejs/node_modules/sharedMemory`