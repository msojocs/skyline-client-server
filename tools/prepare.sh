#!/bin/bash
set -euxo pipefail

root_dir=$(cd "$(dirname "$0")/.." && pwd -P)
package_dir="$root_dir/packages/nwjs"
node_modules_dir="$package_dir/node_modules"
cache_dir="$package_dir/cache"

sharedMemory_version=$(node $root_dir/tools/parse-config.js --get-shared-memory-version)
devtools_version=$(node $root_dir/tools/parse-config.js --get-devtools-version)

# devtools: download if not cached
devtools_cache="$cache_dir/devtools-${devtools_version}.exe"
mkdir -p "$cache_dir"
if [ ! -f "$devtools_cache" ]; then
    wget -c "https://servicewechat.com/wxa-dev-logic/download_redirect?type=win32_x64&from=mpwiki&download_version=${devtools_version}&version_type=1" \
        -O "$devtools_cache"
fi

# sharedMemory
mkdir -p "$node_modules_dir/sharedMemory"
wget -c "https://github.com/msojocs/skyline-shared-memory/releases/download/v${sharedMemory_version}/skyline-sharedMemory-win32-x86_64-v${sharedMemory_version}.node" \
    -O "$node_modules_dir/sharedMemory/sharedMemory.node"

# documentstart/index.js
mkdir -p "$package_dir/documentstart"
7z x "$devtools_cache" -aoa -o"$package_dir/documentstart" \
    "code/package.nw/js/extensions/inject/documentstart/index.js"
mv "$package_dir/documentstart/code/package.nw/js/extensions/inject/documentstart/index.js" \
    "$package_dir/documentstart/index.js"
rm -rf "$package_dir/documentstart/code"

# skyline-addon
rm -rf "$node_modules_dir/skyline-addon"
mkdir -p "$node_modules_dir/skyline-addon"
7z x "$devtools_cache" -aoa -o"$node_modules_dir/skyline-addon" \
    "code/package.nw/node_modules/skyline-addon"
mv "$node_modules_dir/skyline-addon/code/package.nw/node_modules/skyline-addon/"* \
    "$node_modules_dir/skyline-addon/"
rm -rf "$node_modules_dir/skyline-addon/code"
