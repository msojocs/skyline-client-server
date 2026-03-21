#!/bin/bash
set -ex

root_dir=$(cd "$(dirname "$0")/.." && pwd -P)

devtools_version=$(node $root_dir/tools/parse-config.js --get-devtools-version)
image_version=$(node $root_dir/tools/parse-config.js --get-image-version)

docker build \
    -t devtools-server:$image_version \
    -f $root_dir/Dockerfile \
    --build-arg DEVTOOLS_VERSION=$devtools_version \
    $root_dir
