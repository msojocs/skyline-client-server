#!/bin/bash
set -ex

script_dir=$(cd "$(dirname "$0")" && pwd -P)
repo_root=$(cd "$script_dir/.." && pwd -P)

docker build -t ghcr.io/msojocs/skyline-client-server:debug -f "$repo_root/Dockerfile" "$repo_root"

docker run -it \
  --restart=always \
  --hostname="$(hostname)" \
  --env="DISPLAY" \
  --platform="linux/amd64" \
  --volume="${XAUTHORITY:-${HOME}/.Xauthority}:/root/.Xauthority:ro" \
  --volume="/tmp/.X11-unix:/tmp/.X11-unix:ro" \
  --volume="/dev/shm:/dev/shm" \
  -p 3001:3001 \
  --name skyline_server \
  ghcr.io/msojocs/skyline-client-server:debug
