#!/bin/bash
set -ex

# docker build -t skyline-server .
if [ ! -f "ucrtbased.dll" ];then
  wget -c https://github.com/msojocs/skyline-client-server/releases/download/dll/ucrtbased.dll -O ucrtbased.dll.tmp
  mv ucrtbased.dll.tmp ucrtbased.dll
fi

if [ ! -f "vcruntime140d.dll" ];then
  wget -c https://github.com/msojocs/skyline-client-server/releases/download/dll/vcruntime140d.dll -O vcruntime140d.dll.tmp
  mv vcruntime140d.dll.tmp vcruntime140d.dll
fi

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
  --volume=".:/workspace" \
  -w /workspace \
  -p 3001:3001 \
  scottyhardy/docker-wine:latest \
  /bin/bash -c "wine node.exe server.js"