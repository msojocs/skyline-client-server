#!/bin/bash
set -euxo pipefail

root_dir=$(cd "$(dirname "$0")/../.." && pwd -P)
package_dir="$root_dir/packages/nodejs"
node_modules_dir="$package_dir/node_modules"
cache_dir="$package_dir/cache"

sharedMemory_version="${SHARED_MEMORY_VERSION:-v1.0.3}"
devtools_version="${DEVTOOLS_VERSION:-2012510280}"

# skyline server ts
cp "$root_dir/ts-linux-artifact/server.js" "$package_dir/server.js"

# skyline server native
mkdir -p "$node_modules_dir/skyline-server"
cp "$root_dir/native-win-artifact"/*.node "$node_modules_dir/skyline-server/server.node"
cp "$root_dir/native-win-artifact"/*.dll "$package_dir"
