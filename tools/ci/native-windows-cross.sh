#!/bin/bash
set -ex
root_dir=$(cd `dirname $0`/../.. && pwd -P)

arch=$1
tag=$2

# Cross-compile for Windows using MinGW-w64 toolchain
cmake --no-warn-unused-cli \
    -DCMAKE_BUILD_TYPE:STRING=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE \
    -DCMAKE_TOOLCHAIN_FILE="$root_dir/packages/native/cmake/x86_64-w64-mingw32.cmake" \
    -S"$root_dir/packages/native" \
    -B"$root_dir/build-win" \
    -G Ninja

cmake --build "$root_dir/build-win" --config Release --target server --
cmake --build "$root_dir/build-win" --config Release --target nw --
cmake --build "$root_dir/build-win" --config Release --target node --

mkdir -p "$root_dir/tmp/build"

mv "$root_dir/packages/nwjs/node_modules/skyline-server/server.node" "$root_dir/tmp/build/skyline-server-win32-${arch}-${tag}.node"
mv "$root_dir/packages/nwjs/node.dll" "$root_dir/tmp/build/node.dll"
mv "$root_dir/packages/nwjs/nw.dll" "$root_dir/tmp/build/nw.dll"
