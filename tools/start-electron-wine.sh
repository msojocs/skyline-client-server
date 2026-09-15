#!/bin/bash
# 使用 wine 启动 Windows 版 Electron
# 用法: bash tools/start-electron-wine.sh [额外参数...]
set -e

root_dir=$(cd "$(dirname "$0")/.." && pwd -P)
cd "$root_dir"

ELECTRON_DIR="cache/electron-win32-x64"
APP_DIR="packages/electron"

# 下载脚本会检查来源和文件完整性，自动替换旧的官方 Electron 缓存。
bash tools/download-electron-win.sh

if ! command -v wine >/dev/null 2>&1; then
    echo "错误: 未找到 wine，请先安装 wine。" >&2
    exit 1
fi

# 微信安装包的 resources 不参与运行；Electron 直接加载本项目的应用入口。
mkdir -p "$ELECTRON_DIR/resources"
ln -sfnT "../../../$APP_DIR" "$ELECTRON_DIR/resources/app"

export WINEDEBUG=-all

# 无显示环境时使用 xvfb-run
if [ -z "$DISPLAY" ] && command -v xvfb-run >/dev/null 2>&1; then
    echo "未检测到 DISPLAY，使用 xvfb-run 启动..."
    exec xvfb-run -a wine "$ELECTRON_DIR/electron.exe" --remote-debugging-port=9222 "$@"
fi

exec wine "$ELECTRON_DIR/electron.exe" --remote-debugging-port=9222 "$@"
