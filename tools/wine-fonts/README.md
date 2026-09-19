# Skyline Wine 字体资源：Noto Color Emoji

模拟器的 emoji 来源已改为编辑器实际使用的 **Noto Color Emoji 2.051**。通过 Linux 编辑器的 `CSS.getPlatformFontsForNode` 确认了 🍎、🚜 的实际字体为 `Noto Color Emoji (Fontations)`；源文件是 `/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf`。

`SkylineFallback.ttf` 的字体族仍叫 `Skyline Fallback`。中文来自 Droid Sans Fallback，普通符号由 Symbola 补充，emoji 使用 Noto 的原始 PNG 图像。源 Noto 字体采用 CBDT/CBLC；生成器将图像及其位置、分辨率封装到 `sbix`，不重新绘制、不量化颜色。共 42,206 个字形、38,533 个 Unicode 码位、4,070 个位图字形，保留源字体的 GSUB 与 variation selector 规则。包括肤色、ZWJ、国旗和键帽组合。

Segoe 不再提供 emoji 图案；其现有资源仅用于保留上一版复合字体的五个非 emoji 兼容符号（U+3244–U+3247、U+F8FF），因此原版字符覆盖没有减少。Noto 的来源和许可见 [NotoColorEmoji.COPYRIGHT](NotoColorEmoji.COPYRIGHT)。

## 重建字体

需要 Python、`fonttools==4.65.0`，以及 `fonts-droid-fallback` 包中的 `DroidSansFallbackFull.ttf`。其余源字体在本目录内，生成器校验四个输入文件的 SHA-256。

```bash
python3 -m venv cache/font-builder
cache/font-builder/bin/pip install fonttools==4.65.0
cache/font-builder/bin/python tools/wine-fonts/build-color-font.py
```

可指定 `--text-font /path/to/DroidSansFallbackFull.ttf` 或 `--output /path/to/result.ttf`。相同输入与 `SOURCE_DATE_EPOCH` 会生成相同二进制。构建的临时目录仅存放字体中间产物，不涉及 Wine 或 Electron 用户数据。输出采用原子 rename，避免截断进程正在映射的字体。

源 Droid 文件的 SHA-256：`acb6440a713d880a13a21b468ba7cd43f5a2b2934972e51be791c880730777b8`。

## Wine 11.0 的位图读取支持

`dwrite.dll` 和 `dwrite.so` 配套使用，针对 64 位 Wine 11.0 构建：

- [dwrite-analyzer.patch](dwrite-analyzer.patch) 是此前的字体回退补丁。
- [dwrite-bitmap.patch](dwrite-bitmap.patch) 增加 sbix PNG 的颜色字体识别、`GetGlyphImageFormats`、`GetGlyphImageData` 和 `ReleaseGlyphImageData`。读取时验证表偏移、长度、PNG 签名和图像尺寸，选取最接近请求分辨率的 strike，并保留字体表至调用方释放。
- Unix 侧让 FreeType 忽略 sbix，仅负责可缩放的中文／拉丁轮廓与设计度量。emoji 位图通过上述 DirectWrite 接口交给 Skia，避免一个混合字体被当成只能按固定尺寸绘制的位图字体。

构建依赖包括 GCC、MinGW x86-64 GCC、make、flex、bison 和 FreeType 开发头文件。**必须启用 FreeType**；禁用它会导致普通文字无法栅格化。基于 Wine 11.0 原始源码：

```bash
# 在 Wine 11.0 源码根目录，FONT_RESOURCES 指向本目录的绝对路径。
patch -p1 < "$FONT_RESOURCES/dwrite-analyzer.patch"
patch -p1 < "$FONT_RESOURCES/dwrite-bitmap.patch"
./configure --enable-win64 --without-x --without-wayland --without-oss \
  --without-pulse --without-gstreamer --without-vulkan --without-cups \
  --without-sane --without-usb --without-udev --without-pcap --without-netapi
make -C dlls/dwrite -j4
```

产物为 `dlls/dwrite/x86_64-windows/dwrite.dll` 和 `dlls/dwrite/dwrite.so`。替换本目录产物前停止 Skyline；不要直接覆盖正在被进程映射的库文件。

本机启动脚本先进入 `bwrap` 的模块覆盖环境，再执行字体注册和 Electron。这样 `reg.exe` 启动的 wineserver 也能看到配套的 DirectWrite 模块。绑定为只读，系统 Wine 安装不变，继续使用现有 WINEPREFIX。可设置 `SKYLINE_WINE_FONT_FIX=0` 关闭本机修复。

## Docker 集成

[Dockerfile](../../Dockerfile) 固定安装 WineHQ stable `11.0.0.0~noble-1`，在首次运行 Wine 前将本目录的 `dwrite.dll`、`dwrite.so` 复制到 `/opt/wine-stable/lib/wine/` 对应的 Windows／Unix 模块目录。容器直接使用镜像内的模块，无需 `bwrap`。

运行时字体 `SkylineFallback.ttf`、`Symbola.ttf` 和 `seguiemj.ttf` 安装到 `/usr/local/share/fonts/skyline/` 并刷新 fontconfig 缓存；Noto 的许可文件随镜像保留。`NotoColorEmoji.ttf` 是生成字体时的输入，运行时使用已嵌入 `SkylineFallback.ttf` 的 PNG。

[容器启动脚本](../xvfb-startup.sh) 在 Xvfb 启动后、Electron 启动前，以 `docker` 用户将上述字体路径写入当前 Wine prefix 的 `HKLM\Software\Microsoft\Windows NT\CurrentVersion\Fonts`，使挂载的已有 prefix 也使用镜像内的字体。更新本目录的模块或字体后，需要重新构建镜像并重新创建容器。

脚本通过 `script` 为 Wine/Electron 提供伪终端，避免 Wine 11.0 在 Docker 日志管道下触发 Node.js 的 `stdout: open EBADF`。日志仍输出到容器日志，Electron 的退出码由 `script -e` 传回；使用 `docker run -d` 时无需额外加 `-t`。

## 验证

[bitmap-probe.c](test/bitmap-probe.c) 在真实 Wine API 上验证中文、ASCII、数字、符号的设计度量，三个 emoji 的 PNG 图像、基线和分辨率，重复申请／释放，以及非法 glyph 和参数处理。

```bash
x86_64-w64-mingw32-gcc -Wall -O2 tools/wine-fonts/test/bitmap-probe.c \
  -o cache/bitmap-probe.exe -ldwrite -lole32
# 必须在与启动脚本相同的 bwrap 模块覆盖环境内执行：
# wine cache/bitmap-probe.exe [输出的拖拉机PNG路径]
```

原生接口返回的 🚜 PNG 已与 Noto 源字体逐字节比对。真实页面与编辑器证据见 [Noto 验证记录](../../docs/skyline-font-fallback/noto-emoji/README.md)。

Docker 中也通过实际容器启动脚本运行了该探针，验证新建 prefix，以及将已有 prefix 的旧字体路径更新为镜像路径。两种情况均通过；容器返回的 🚜 PNG SHA-256 为 `2d1feefb74fe48bcbf9d270f6f3830b64dd0739c2739fb318ba4d5db52d95b09`，与 Noto 源图一致。

## 校验值

```text
SkylineFallback.ttf  130e698b42dc0b2474a1cbf094f36dfe78d9d58d7d17a9405b9f58db45330ce8
NotoColorEmoji.ttf   9fd0a3d0ce84d77e3185dfbae77bd1abf3926aa49a032e354d076c4f17151f10
Symbola.ttf          af8bd8cc9c808272034651cb5ebf6c38370ae536fe631eb89462f127f11e793f
seguiemj.ttf         f07cbd7886f4a1a5255a1bdf4ca5ea29db3be3357414003a92bec5d1c3165578
dwrite.dll          d7423154540d285790db3f047af2093fd8ddf470bfaa4c41bf4f4eb453b05863
dwrite.so           6f9c33847eb3974a66a3aa9e5e7489e58ca112557d702374c5869effe2d33a84
```
