#!/bin/bash
# 下载 Windows 版 Electron 并解压到 cache/electron-win32-x64
# 用法: bash tools/download-electron-win.sh
set -e

root_dir=$(cd "$(dirname "$0")/.." && pwd -P)
cd "$root_dir"

ELECTRON_VERSION="36.6.0"
ZIP_FILE="cache/electron-v${ELECTRON_VERSION}-win32-x64.zip"
DEST_DIR="cache/electron-win32-x64"
DOWNLOAD_URL="https://github.com/electron/electron/releases/download/v${ELECTRON_VERSION}/electron-v${ELECTRON_VERSION}-win32-x64.zip"

if [ -f "$DEST_DIR/electron.exe" ]; then
    echo "Windows Electron ${ELECTRON_VERSION} 已存在于 $DEST_DIR，跳过下载。"
    exit 0
fi

mkdir -p cache

# 防止并发运行损坏下载文件
exec 9>"$ZIP_FILE.lock"
flock -n 9 || { echo "错误: 已有另一个下载任务正在进行。" >&2; exit 1; }

if [ -f "$ZIP_FILE" ] && ! unzip -t "$ZIP_FILE" >/dev/null 2>&1; then
    echo "检测到不完整/损坏的 zip，删除后重新下载..."
    rm -f "$ZIP_FILE"
fi

if [ ! -f "$ZIP_FILE" ]; then
    echo "下载 $DOWNLOAD_URL ..."
    wget -c "$DOWNLOAD_URL" -O "$ZIP_FILE"
fi

echo "校验 zip 完整性..."
unzip -t "$ZIP_FILE" >/dev/null

echo "解压到 $DEST_DIR ..."
mkdir -p "$DEST_DIR"
unzip -o "$ZIP_FILE" -d "$DEST_DIR"

echo "完成: $DEST_DIR/electron.exe"
