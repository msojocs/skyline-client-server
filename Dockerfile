FROM ubuntu:24.04 AS runtime-base

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV XVFB_RES="800x600x24"
ENV XVFB_ARGS=""
ENV WINEDLLOVERRIDES="mscoree,mshtml="
# Force Mesa software rasterizer — no GPU in container
ENV LIBGL_ALWAYS_SOFTWARE=1
ENV GALLIUM_DRIVER=llvmpipe

RUN sed -i 's|http://archive.ubuntu.com|http://mirrors.aliyun.com|g' /etc/apt/sources.list.d/ubuntu.sources && \
    sed -i 's|http://security.ubuntu.com|http://mirrors.aliyun.com|g' /etc/apt/sources.list.d/ubuntu.sources && \
    apt update && \
    apt install -y fonts-noto-cjk fontconfig libfreetype6 sudo wget gnupg libgl1 && \
    apt clean && \
    rm -rf /var/lib/apt/lists/*

# The local DirectWrite PE/Unix pair is built against Wine 11.0.
ARG WINE_VERSION="11.0.0.0~noble-1"
RUN mkdir -pm755 /etc/apt/keyrings && \
    wget -nv -O /etc/apt/keyrings/winehq-archive.key https://dl.winehq.org/wine-builds/winehq.key && \
    echo "deb [signed-by=/etc/apt/keyrings/winehq-archive.key] https://dl.winehq.org/wine-builds/ubuntu/ $(grep VERSION_CODENAME= /etc/os-release | cut -d= -f2) main" > /etc/apt/sources.list.d/winehq.list && \
    dpkg --add-architecture i386 && \
    apt update && \
    apt install -y --install-recommends \
        winehq-stable="${WINE_VERSION}" \
        wine-stable="${WINE_VERSION}" \
        wine-stable-amd64="${WINE_VERSION}" \
        wine-stable-i386:i386="${WINE_VERSION}" && \
    apt install -y --no-install-recommends xvfb libegl1 libegl-mesa0 libglx-mesa0 mesa-vulkan-drivers mesa-utils gosu && \
    rm -rf /var/lib/apt/lists/*

# Install both halves before the first Wine process creates a prefix or caches
# builtin module handles. The container has its own Wine installation, so it
# does not need the host launcher's bwrap overlay.
COPY --chmod=644 tools/wine-fonts/dwrite.dll /opt/wine-stable/lib/wine/x86_64-windows/dwrite.dll
COPY --chmod=644 tools/wine-fonts/dwrite.so /opt/wine-stable/lib/wine/x86_64-unix/dwrite.so
COPY --chmod=644 tools/wine-fonts/SkylineFallback.ttf tools/wine-fonts/Symbola.ttf tools/wine-fonts/seguiemj.ttf /usr/local/share/fonts/skyline/
COPY --chmod=644 tools/wine-fonts/NotoColorEmoji.COPYRIGHT /usr/share/doc/skyline-fonts/
RUN chmod 755 /usr/local/share/fonts/skyline /usr/share/doc/skyline-fonts && \
    fc-cache -f /usr/local/share/fonts/skyline && \
    test "$(wine --version)" = "wine-11.0"

RUN useradd -m docker && \
    echo "docker:docker" | chpasswd && \
    adduser docker sudo && \
    echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers && \
    chmod -R a+X /usr/share/fonts

FROM node:20-bookworm AS server-builder

WORKDIR /build
RUN corepack enable

COPY package.json pnpm-lock.yaml pnpm-workspace.yaml ./
COPY packages/typescript/package.json ./packages/typescript/package.json
# Bundling only needs the package's types; electron-builder supplies the runtime.
RUN ELECTRON_SKIP_BINARY_DOWNLOAD=1 pnpm install --frozen-lockfile --filter ./packages/typescript

COPY packages/typescript ./packages/typescript
RUN pnpm --filter ./packages/typescript build

FROM ubuntu:22.04 AS skyline-addon-builder

ARG DEVTOOLS_VERSION
# The current installer is published at this direct URL; its redirect endpoint
# does not expose a Location header in the container environment.
ARG DEVTOOLS_URL="https://devtools.wxqcloud.qq.com.cn/WechatWebDev/release/be1ec64cf6184b0fa64091919793f068/wechat_devtools_2.02.2608070_win32_x64.exe"
WORKDIR /build
RUN sed -i 's/security.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list && \
    sed -i 's/archive.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list && \
    apt update && \
    apt install -y wget p7zip-full && \
    apt clean && \
    rm -rf /var/lib/apt/lists/*
RUN mkdir -p cache node_modules/skyline-addon && \
    wget -c "${DEVTOOLS_URL}" -O "cache/devtools-${DEVTOOLS_VERSION}.exe" && \
    7z x "cache/devtools-${DEVTOOLS_VERSION}.exe" -aoa -onode_modules/skyline-addon resources/app.asar.unpacked/node_modules/skyline-addon && \
    mv node_modules/skyline-addon/resources/app.asar.unpacked/node_modules/skyline-addon/* node_modules/skyline-addon/ && \
    rm -rf node_modules/skyline-addon/resources cache

FROM node:20-bookworm AS electron-builder

WORKDIR /build
RUN apt-get update && \
    apt-get install -y --no-install-recommends curl p7zip-full && \
    rm -rf /var/lib/apt/lists/*
COPY package.json ./
COPY config/config.json ./config/config.json
COPY tools/parse-config.js tools/download-electron-win.sh ./tools/
RUN bash tools/download-electron-win.sh

FROM ubuntu:22.04 AS source

# Keep in sync with config/config.json; build-docker-image.sh passes the
# configured value. The pre-1.1.0 Windows builds import node.dll, which does
# not exist under Electron (or Wine), so sharedMemory.node failed to load.
ARG SHARED_MEMORY_VERSION="1.1.0"
WORKDIR /workspace
COPY --from=electron-builder /build/cache/electron-win32-x64 electron
RUN sed -i 's/security.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list && \
    sed -i 's/archive.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list && \
    apt update && apt install -y wget && \
    rm -rf /var/lib/apt/lists/*
# Vite emits the renderer server and the combined Electron main-process server
# into the same package directory.
COPY --from=server-builder /build/packages/electron/render-server.js electron/resources/app/
COPY --from=server-builder /build/packages/electron/main-server.js electron/resources/app/
COPY packages/electron electron/resources/app
COPY --from=skyline-addon-builder /build/node_modules/skyline-addon electron/resources/app/node_modules/skyline-addon
RUN rm -rf electron/resources/app/cache electron/resources/app/node_modules/sharedMemory && \
    mkdir -p electron/resources/app/node_modules/sharedMemory && \
    wget -c "https://github.com/msojocs/skyline-shared-memory/releases/download/v${SHARED_MEMORY_VERSION}/skyline-sharedMemory-win32-x86_64-v${SHARED_MEMORY_VERSION}.node" -O electron/resources/app/node_modules/sharedMemory/sharedMemory.node && \
    chmod -R a+X electron

FROM runtime-base AS runtime
WORKDIR /workspace
COPY --from=source /workspace/electron /workspace
COPY packages/electron/node_modules/skyline-server /workspace/resources/app/node_modules/skyline-server
COPY tools/xvfb-startup.sh xvfb-startup.sh

EXPOSE 9222
CMD ["/workspace/xvfb-startup.sh"]
