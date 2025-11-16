#!/bin/bash
set -ex
root_dir=$(cd `dirname $0`/../.. && pwd -P)

# skyline server ts
cd "$root_dir/ts-linux-artifact"
cp server.js "$root_dir/packages/nodejs"

# skyline server native
cd "$root_dir/native-win-artifact"
mkdir -p "$root_dir/packages/nodejs/node_modules/skyline-server"
cp *.node "$root_dir/packages/nodejs/node_modules/skyline-server/server.node"
cp *.dll "$root_dir/packages/nodejs"

cd "$root_dir/packages/nodejs"
if [ ! -f "ucrtbased.dll" ];then
  wget -c https://github.com/msojocs/skyline-client-server/releases/download/dll/ucrtbased.dll -O ucrtbased.dll.tmp
  mv ucrtbased.dll.tmp ucrtbased.dll
fi

if [ ! -f "vcruntime140d.dll" ];then
  wget -c https://github.com/msojocs/skyline-client-server/releases/download/dll/vcruntime140d.dll -O vcruntime140d.dll.tmp
  mv vcruntime140d.dll.tmp vcruntime140d.dll
fi

if [ ! -f "node.exe" ];then
  wget -c https://github.com/msojocs/skyline-node/releases/download/v16.4.0-1/node.exe -O node.exe.tmp
  mv node.exe.tmp node.exe
fi

cd "$root_dir/packages/nodejs/node_modules"
# SharedMemory
mkdir -p sharedMemory
sharedMemory_version="1.0.2"
if [ ! -f sharedMemory/sharedMemory.node ];then
    wget -c https://github.com/msojocs/skyline-shared-memory/releases/download/v1.0.2/skyline-sharedMemory-win32-x86_64-v1.0.2.node -O sharedMemory/sharedMemory.node
fi

cd "$root_dir/packages/nodejs"
# skyline module
mkdir -p cache
# 下载wechatdevtools
devtools_version="2012510241"
if [ ! -f "devtools-$devtools_version.exe" ];then
    wget -c "https://servicewechat.com/wxa-dev-logic/download_redirect?type=win32_x64&from=mpwiki&download_version=$devtools_version&version_type=1" -O "cache/devtools-$devtools_version.exe"
fi
# 解压skyline-addon
rm -rf node_modules/skyline-addon
7z x "cache/devtools-$devtools_version.exe" -aoa -onode_modules/skyline-addon code/package.nw/node_modules/skyline-addon
cd node_modules/skyline-addon
mv code/package.nw/node_modules/skyline-addon/* .
rm -rf code