#!/bin/bash
set -ex

# docker build -t ghcr.io/msojocs/skyline-client-server:debug .
if [ ! -f "ucrtbased.dll" ];then
  wget -c https://github.com/msojocs/skyline-client-server/releases/download/dll/ucrtbased.dll -O ucrtbased.dll.tmp
  mv ucrtbased.dll.tmp ucrtbased.dll
fi

if [ ! -f "vcruntime140d.dll" ];then
  wget -c https://github.com/msojocs/skyline-client-server/releases/download/dll/vcruntime140d.dll -O vcruntime140d.dll.tmp
  mv vcruntime140d.dll.tmp vcruntime140d.dll
fi

docker run -it \
  --restart=always \
  --hostname="$(hostname)" \
  --env="DISPLAY" \
  --platform="linux/amd64" \
  --volume="${XAUTHORITY:-${HOME}/.Xauthority}:/root/.Xauthority:ro" \
  --volume="/tmp/.X11-unix:/tmp/.X11-unix:ro" \
  --volume="/dev/shm:/dev/shm" \
  -p 3001:3001 \
  --name skyline_server \
  ghcr.io/msojocs/skyline-client-server:debug