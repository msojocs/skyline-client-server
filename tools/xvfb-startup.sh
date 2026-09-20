#!/bin/bash
# from https://github.com/bengreenier/docker-xvfb/blob/master/docker/xvfb-startup.sh
set -e

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
trap 'kill "$XVFB_PROC" 2>/dev/null || true' EXIT
sleep 1
export DISPLAY=:99
export LANG=zh_CN.UTF-8
export LC_ALL=zh_CN.UTF-8
export LANGUAGE=zh_CN.UTF-8
export WINEDEBUG=${WINEDEBUG:--all}

# Wine DirectWrite enumerates HKLM fonts. Register the image's font paths on
# every start so an existing, mounted prefix also sees the current resources.
font_registry='HKLM\Software\Microsoft\Windows NT\CurrentVersion\Fonts'
gosu docker wine reg add "$font_registry" /v 'Skyline Fallback (TrueType)' \
    /t REG_SZ /d 'Z:\usr\local\share\fonts\skyline\SkylineFallback.ttf' /f >/dev/null
gosu docker wine reg add "$font_registry" /v 'Symbola (TrueType)' \
    /t REG_SZ /d 'Z:\usr\local\share\fonts\skyline\Symbola.ttf' /f >/dev/null
gosu docker wine reg add "$font_registry" /v 'Segoe UI Emoji (TrueType)' \
    /t REG_SZ /d 'Z:\usr\local\share\fonts\skyline\seguiemj.ttf' /f >/dev/null

cd /workspace
set +e
# Wine 11.0 needs a terminal for Node's stdout handles. A detached Docker log
# pipe otherwise makes Electron fail with "open EBADF" before the app starts.
script -q -e -f -c 'gosu docker wine electron.exe --remote-debugging-port=9222 --disable-gpu' /dev/null
WINE_EXIT=$?
exit $WINE_EXIT
