#!/bin/bash
# from https://github.com/bengreenier/docker-xvfb/blob/master/docker/xvfb-startup.sh

# Create X11 socket directory as root before dropping privileges
mkdir -p /tmp/.X11-unix
chmod 1777 /tmp/.X11-unix

HOST_UID=${HOST_UID:-1000}
HOST_GID=${HOST_GID:-1000}

# Remap the docker user to match host UID/GID so shared memory has correct ownership.
# If the target GID/UID is already taken by another user/group, relocate it first
# to avoid a silent failure (e.g. host UID=1000 collides with the 'ubuntu' user inside
# the container).
CONFLICT_GROUP=$(getent group "$HOST_GID" | cut -d: -f1)
if [ -n "$CONFLICT_GROUP" ] && [ "$CONFLICT_GROUP" != "docker" ]; then
    groupmod -g "$(( HOST_GID + 60000 ))" "$CONFLICT_GROUP" 2>/dev/null || true
fi

CONFLICT_USER=$(getent passwd "$HOST_UID" | cut -d: -f1)
if [ -n "$CONFLICT_USER" ] && [ "$CONFLICT_USER" != "docker" ]; then
    usermod -u "$(( HOST_UID + 60000 ))" "$CONFLICT_USER" 2>/dev/null || true
fi

groupmod -g "$HOST_GID" docker 2>/dev/null || true
usermod -u "$HOST_UID" docker 2>/dev/null || true
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