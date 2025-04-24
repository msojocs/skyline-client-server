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

# skyline server ts
cd "$root_dir/ts-linux-artifact"
mv server.js "$root_dir/packages/nwjs/webview"

# skyline server native
cd "$root_dir/native-win-artifact"
mkdir -p "$root_dir/packages/nwjs/node_modules/skyline-server"
mv *.node "$root_dir/packages/nwjs/node_modules/skyline-server/skylineServer.node"

#pack
cd "$root_dir/packages"
tar -zcf "$root_dir/tmp/upload/skyline-skylineServer-win32-$arch-$tag.tar.gz" nwjs