#!/bin/bash
root_dir=$(cd `dirname $0`/.. && pwd -P)

server_dir="$root_dir/packages/electron"

mkdir -p "$server_dir"
mkdir -p "$server_dir/node_modules/skyline-server" "$server_dir/node_modules/sharedMemory"
cp "$root_dir/native-win-artifact"/*.node "$server_dir/node_modules/skyline-server/server.node"
cp "/home/msojocs/github/skyline-shared-memory/build/sharedMemory.node" "$server_dir/node_modules/sharedMemory/sharedMemory.node"
cd "$root_dir"
pnpm exec electron packages/electron > font.log 2>&1
