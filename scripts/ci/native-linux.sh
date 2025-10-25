#!/bin/bash
set -ex
root_dir=$(cd `dirname $0`/../.. && pwd -P)

arch=$1
tag=$2

cmake --no-warn-unused-cli -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -S"$root_dir/packages/native" -B"$root_dir/build" -G Ninja
cmake --build "$root_dir/build" --config Release --target skyline --

mkdir -p "$root_dir/tmp/build"

mv "$root_dir/packages/native/build/skyline.node" "$root_dir/tmp/build/skyline-skylineClient-linux-$arch-$tag.node"