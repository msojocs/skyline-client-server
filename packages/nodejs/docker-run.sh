#!/bin/bash

# docker build -t skyline-server .

docker run -it \
  --rm \
  --hostname="$(hostname)" \
  -e USER_NAME="docker" \
  -e USER_UID="$(id -u)" \
  -e USER_GID="$(id -g)" \
  --env="DISPLAY" \
  --platform="linux/amd64" \
  --volume="${XAUTHORITY:-${HOME}/.Xauthority}:/root/.Xauthority:ro" \
  --volume="/tmp/.X11-unix:/tmp/.X11-unix:ro" \
  --volume="/dev/shm:/dev/shm" \
  -p 3001:3001 \
  skyline-server