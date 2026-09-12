#!/bin/bash
set -ex
root_dir=$(cd `dirname $0`/../.. && pwd -P)
arch=$1
tag=$2

# 下载Artifact之后打包

mkdir -p "$root_dir/tmp/upload"

# skyline client
cd "$root_dir/native-linux-artifact"
mv *.node "$root_dir/tmp/upload"

$root_dir/tools/prepare.sh

# skyline server ts
cd "$root_dir/ts-linux-artifact"
mv server.js main.js main-rpc.js "$root_dir/packages/electron"

# skyline server native
cd "$root_dir/native-win-artifact"
mkdir -p "$root_dir/packages/electron/node_modules/skyline-server"
mv skyline-server-win32-*.node "$root_dir/packages/electron/node_modules/skyline-server/render-server.node"
mv skyline-client-win32-*.node "$root_dir/tmp/upload"

#pack
cd "$root_dir/packages"
rm -rf electron/.gitignore electron/README.MD electron/log electron/cache electron/run*
tar -zcf "$root_dir/tmp/upload/skyline-server-win32-$arch-$tag.tar.gz" electron
