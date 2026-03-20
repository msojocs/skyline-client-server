#!/bin/bash
# from https://github.com/bengreenier/docker-xvfb/blob/master/docker/xvfb-startup.sh

# Create X11 socket directory as root before dropping privileges
mkdir -p /tmp/.X11-unix
chmod 1777 /tmp/.X11-unix

HOST_UID=${HOST_UID:-1000}
HOST_GID=${HOST_GID:-1000}

# Remap the docker user to match host UID/GID so shared memory has correct ownership
groupmod -g "$HOST_GID" docker 2>/dev/null || true
usermod -u "$HOST_UID" docker 2>/dev/null || true
chown -R docker /home/docker 2>/dev/null || true
chown -R docker /workspace 2>/dev/null || true

rm -rf /tmp/.X99-lock
Xvfb :99 -ac -screen 0 "$XVFB_RES" -nolisten tcp $XVFB_ARGS &
XVFB_PROC=$!
sleep 1
export DISPLAY=:99
cd /workspace
gosu docker wine nw.exe --remote-debugging-port=9222 "--load-extension=/workspace/WeappPlugin" "--custom-devtools-frontend=/workspace/inspector"
WINE_EXIT=$?
kill $XVFB_PROC
exit $WINE_EXIT