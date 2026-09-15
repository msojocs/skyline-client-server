#!/bin/bash
# 从微信开发者工具安装包提取 Windows Electron 到 cache/electron-win32-x64
# 用法: bash tools/download-electron-win.sh
set -euo pipefail

root_dir=$(cd "$(dirname "$0")/.." && pwd -P)
cd "$root_dir"

DEVTOOLS_VERSION=$(node tools/parse-config.js --get-electron-devtools-version)
DOWNLOAD_URL=$(node tools/parse-config.js --get-electron-url)
INSTALLER_FILE="cache/wechat_devtools_${DEVTOOLS_VERSION}_x64.exe"
DEST_DIR="cache/electron-win32-x64"
SOURCE_FILE="$DEST_DIR/.devtools-source"

runtime_complete() {
    local directory="$1" file
    for file in electron.exe chrome_100_percent.pak chrome_200_percent.pak \
        d3dcompiler_47.dll ffmpeg.dll icudtl.dat libEGL.dll libGLESv2.dll \
        resources.pak snapshot_blob.bin v8_context_snapshot.bin \
        locales/en-US.pak locales/zh-CN.pak; do
        [ -s "$directory/$file" ] || return 1
    done
    return 0
}

mkdir -p cache

# Linux 本地启动和 Docker 共用固定目标目录的锁；Windows Git Bash 无 flock。
if command -v flock >/dev/null 2>&1; then
    exec 9>"$DEST_DIR.lock"
    flock -n 9 || { echo "错误: 已有另一个下载任务正在进行。" >&2; exit 1; }
fi

if [ -f "$SOURCE_FILE" ] && [ "$(cat "$SOURCE_FILE")" = "$DOWNLOAD_URL" ] && runtime_complete "$DEST_DIR"; then
    echo "微信开发者工具 ${DEVTOOLS_VERSION} 的 Windows Electron 已存在于 $DEST_DIR，跳过下载。"
    exit 0
fi

for dependency in curl 7z; do
    if ! command -v "$dependency" >/dev/null 2>&1; then
        echo "错误: 未找到 $dependency，请先安装。" >&2
        exit 1
    fi
done

# 同一版本的下载地址也可能变化，安装包和续传文件必须属于当前来源。
if [ ! -f "$INSTALLER_FILE.source" ] || [ "$(cat "$INSTALLER_FILE.source")" != "$DOWNLOAD_URL" ]; then
    rm -f "$INSTALLER_FILE" "$INSTALLER_FILE.tmp"
    printf '%s\n' "$DOWNLOAD_URL" > "$INSTALLER_FILE.source"
fi

if [ -f "$INSTALLER_FILE" ] && ! 7z t "$INSTALLER_FILE" >/dev/null 2>&1; then
    echo "检测到不完整/损坏的安装包，删除后重新下载..."
    rm -f "$INSTALLER_FILE"
fi

if [ ! -f "$INSTALLER_FILE" ]; then
    echo "下载 $DOWNLOAD_URL ..."
    curl --fail --location --retry 3 --continue-at - "$DOWNLOAD_URL" -o "$INSTALLER_FILE.tmp"
    if ! 7z t "$INSTALLER_FILE.tmp" >/dev/null; then
        rm -f "$INSTALLER_FILE.tmp"
        echo "错误: 微信开发者工具安装包校验失败。" >&2
        exit 1
    fi
    mv "$INSTALLER_FILE.tmp" "$INSTALLER_FILE"
fi

extract_dir=$(mktemp -d "$DEST_DIR.tmp.XXXXXX")
trap 'rm -rf "$extract_dir"' EXIT

echo "解压到 $DEST_DIR ..."
# 不递归匹配通配符：只保留根目录运行库和 locales，排除 resources/、
# $PLUGINSDIR、CLI、文件监视器等开发工具文件。resources.pak 是运行必需文件。
7z x "$INSTALLER_FILE" -y -o"$extract_dir" \
    '微信开发者工具.exe' '*.dll' '*.pak' '*.bin' '*.dat' \
    'vk_swiftshader_icd.json' 'locales/*'
mv "$extract_dir/微信开发者工具.exe" "$extract_dir/electron.exe"

if ! runtime_complete "$extract_dir"; then
    echo "错误: 安装包中的 Electron 运行文件不完整，保留原有运行时。" >&2
    exit 1
fi
printf '%s\n' "$DOWNLOAD_URL" > "$extract_dir/.devtools-source"

# 解压、校验成功后再替换，避免残留旧版 Electron 或开发工具资源。
rm -rf "$DEST_DIR"
mv "$extract_dir" "$DEST_DIR"

echo "完成: $DEST_DIR/electron.exe"
