#!/bin/bash
set -euo pipefail
root_dir=$(cd "$(dirname "$0")/../.." && pwd -P)
arch=${1:-x86_64}
tag=${2:-continuous}
[[ "$arch" == x86_64 ]] || { echo "Unsupported architecture: $arch" >&2; exit 1; }

node "$root_dir/packages/native/build.js" --target x86_64-unknown-linux-gnu
mkdir -p "$root_dir/tmp/build"
cp "$root_dir/packages/native/build/x86_64-unknown-linux-gnu/skyline.node" \
  "$root_dir/tmp/build/skyline-client-linux-$arch-$tag.node"
