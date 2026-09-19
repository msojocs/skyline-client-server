#!/bin/bash
# 使用 wine 启动 Windows 版 Electron
# 用法: bash tools/start-electron-wine.sh [额外参数...]
set -e

root_dir=$(cd "$(dirname "$0")/.." && pwd -P)
cd "$root_dir"

ELECTRON_DIR="cache/electron-win32-x64"
APP_DIR="packages/electron"

if ! command -v wine >/dev/null 2>&1; then
    echo "错误: 未找到 wine，请先安装 wine。" >&2
    exit 1
fi

# The composite font contains CJK/symbol outlines and original Noto PNGs.
# Load it with the matching Wine 11.0 DirectWrite build. bwrap exposes the
# patched modules only to this process; the existing WINEPREFIX is retained.
WINE_FONT_FIX_DIR="$root_dir/tools/wine-fonts"
WINE_FONT_FIX_DLL="$WINE_FONT_FIX_DIR/dwrite.dll"
WINE_FONT_FIX_UNIX="$WINE_FONT_FIX_DIR/dwrite.so"
WINE_FONT_FIX_TTF="$WINE_FONT_FIX_DIR/seguiemj.ttf"
WINE_SYMBOLA_TTF="$WINE_FONT_FIX_DIR/Symbola.ttf"
WINE_COMPOSITE_TTF="$WINE_FONT_FIX_DIR/SkylineFallback.ttf"
if [[ "${SKYLINE_WINE_FONT_FIX:-1}" != 0 && "${SKYLINE_WINE_FONT_OVERLAY:-}" != "$root_dir" ]]; then
    if ! command -v bwrap >/dev/null 2>&1; then
        echo "错误: Skyline Wine 字体修复需要 bwrap；设置 SKYLINE_WINE_FONT_FIX=0 可显式关闭修复。" >&2
        exit 1
    fi
    wine_binary=$(readlink -f "$(command -v wine)")
    wine_root=$(cd "$(dirname "$wine_binary")/.." && pwd -P)
    wine_dwrite="$wine_root/lib/wine/x86_64-windows/dwrite.dll"
    wine_dwrite_unix="$wine_root/lib/wine/x86_64-unix/dwrite.so"
    if [[ ! -s "$WINE_FONT_FIX_DLL" || ! -s "$WINE_FONT_FIX_UNIX" || ! -f "$wine_dwrite" || ! -f "$wine_dwrite_unix" ]]; then
        echo "错误: 未找到配套的 Wine DirectWrite 模块。" >&2
        exit 1
    fi
    # Enter the overlay before the first Wine command, including reg.exe.
    # Otherwise reg.exe can start a wineserver outside the overlay and its
    # cached builtin DLL handles point subsequent renderers at stock dwrite.
    exec bwrap --dev-bind / / \
        --ro-bind "$WINE_FONT_FIX_DLL" "$wine_dwrite" \
        --ro-bind "$WINE_FONT_FIX_UNIX" "$wine_dwrite_unix" \
        --setenv SKYLINE_WINE_FONT_OVERLAY "$root_dir" \
        -- bash "$root_dir/tools/start-electron-wine.sh" "$@"
fi

# 下载脚本会检查来源和文件完整性，自动替换旧的官方 Electron 缓存。
bash tools/download-electron-win.sh

if [[ "${SKYLINE_WINE_FONT_FIX:-1}" != 0 ]]; then
    if [[ ! -s "$WINE_FONT_FIX_DLL" || ! -s "$WINE_FONT_FIX_UNIX" || ! -s "$WINE_FONT_FIX_TTF" || ! -s "$WINE_SYMBOLA_TTF" || ! -s "$WINE_COMPOSITE_TTF" ]]; then
        echo "错误: 缺少 Skyline Wine 字体修复资源: $WINE_FONT_FIX_DIR" >&2
        exit 1
    fi

    wine_prefix=${WINEPREFIX:-"$HOME/.wine"}
    wine_font_dir="$wine_prefix/drive_c/windows/Fonts"
    mkdir -p "$wine_font_dir"
    install_wine_font() {
        local source=$1 destination=$2 font_tmp
        if cmp -s "$source" "$destination"; then return; fi
        # A renderer may still have the old font mapped. Replace its directory
        # entry atomically instead of truncating the mapped file during update.
        font_tmp=$(mktemp "${destination}.XXXXXX")
        if ! cp "$source" "$font_tmp" || ! chmod 644 "$font_tmp" || ! mv -f "$font_tmp" "$destination"; then
            rm -f "$font_tmp"
            return 1
        fi
    }
    install_wine_font "$WINE_FONT_FIX_TTF" "$wine_font_dir/seguiemj.ttf"
    install_wine_font "$WINE_SYMBOLA_TTF" "$wine_font_dir/Symbola.ttf"
    install_wine_font "$WINE_COMPOSITE_TTF" "$wine_font_dir/SkylineFallback.ttf"
    WINEDEBUG=-all wine reg add \
        'HKCU\Software\Microsoft\Windows NT\CurrentVersion\Fonts' \
        /v 'Segoe UI Emoji (TrueType)' /t REG_SZ \
        /d 'C:\windows\Fonts\seguiemj.ttf' /f >/dev/null
    WINEDEBUG=-all wine reg add \
        'HKLM\Software\Microsoft\Windows NT\CurrentVersion\Fonts' \
        /v 'Segoe UI Emoji (TrueType)' /t REG_SZ \
        /d 'C:\windows\Fonts\seguiemj.ttf' /f >/dev/null
    WINEDEBUG=-all wine reg add \
        'HKCU\Software\Microsoft\Windows NT\CurrentVersion\Fonts' \
        /v 'Symbola (TrueType)' /t REG_SZ \
        /d 'C:\windows\Fonts\Symbola.ttf' /f >/dev/null
    WINEDEBUG=-all wine reg add \
        'HKLM\Software\Microsoft\Windows NT\CurrentVersion\Fonts' \
        /v 'Symbola (TrueType)' /t REG_SZ \
        /d 'C:\windows\Fonts\Symbola.ttf' /f >/dev/null
    WINEDEBUG=-all wine reg add \
        'HKLM\Software\Microsoft\Windows NT\CurrentVersion\Fonts' \
        /v 'Skyline Fallback (TrueType)' /t REG_SZ \
        /d 'C:\windows\Fonts\SkylineFallback.ttf' /f >/dev/null
fi

# 微信安装包的 resources 不参与运行；Electron 直接加载本项目的应用入口。
mkdir -p "$ELECTRON_DIR/resources"
ln -sfnT "../../../$APP_DIR" "$ELECTRON_DIR/resources/app"

export WINEDEBUG=${WINEDEBUG:--all}

wine_command=(wine "$ELECTRON_DIR/electron.exe" --remote-debugging-port=9222 "$@")
# 无显示环境时使用 xvfb-run
if [ -z "$DISPLAY" ] && command -v xvfb-run >/dev/null 2>&1; then
    echo "未检测到 DISPLAY，使用 xvfb-run 启动..."
    exec xvfb-run -a "${wine_command[@]}"
fi

exec "${wine_command[@]}"
