#!/bin/bash
set -ex

root_dir=$(cd "$(dirname "$0")/.." && pwd -P)

docker run --rm -it \
    -p 9222:9222 \
    -e HOST_UID=$(id -u) \
    -e HOST_GID=$(id -g) \
    -v $HOME/.config/wechat-devtools/WeappPlugin:/workspace/WeappPlugin \
    -v $HOME/github/wechat-web-devtools-linux/package.nw/js/ideplugin:/workspace/inspector \
    -v "/dev/shm:/dev/shm" \
    --name wechat_devtools_server \
    test:latest