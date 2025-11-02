#!/bin/bash
root_dir=$(cd `dirname $0`/.. && pwd -P)

server_dir="$root_dir/packages/nodejs"
win_dir="/mnt/d/github/skyline-client-server/packages/nodejs"

mkdir -p "$server_dir"
# 复制更新server文件
if [ ! -f "$server_dir/node.exe" ]; then
    wget -c -O "$server_dir/node.exe" "https://github.com/msojocs/skyline-node/releases/download/v16.4.0-1/node.exe"
fi
if [ ! -f "$server_dir/nw.dll" ]; then
    cp "$win_dir/nw.dll" "$server_dir/nw.dll"
fi
if [ ! -f "$server_dir/node.dll" ]; then
    cp "$win_dir/node.dll" "$server_dir/node.dll"
fi

cp "/mnt/d/github/skyline-client-server/packages/nodejs/node_modules/skyline-server/skylineServer.node" "$server_dir/node_modules/skyline-server/skylineServer.node"

cd "$server_dir"
wine node.exe server.js