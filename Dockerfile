FROM ubuntu:24.04 AS runtime-base

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV XVFB_RES="800x600x24"
ENV XVFB_ARGS=""
ENV WINEDLLOVERRIDES="mscoree,mshtml="
# Force Mesa software rasterizer — no GPU in container
ENV LIBGL_ALWAYS_SOFTWARE=1
ENV GALLIUM_DRIVER=llvmpipe
ADD --chmod=644 https://github.com/msojocs/skyline-client-server/releases/download/dll/seguiemj.ttf /usr/share/fonts/truetype/segoe/seguiemj.ttf

RUN sed -i 's|http://archive.ubuntu.com|http://mirrors.aliyun.com|g' /etc/apt/sources.list.d/ubuntu.sources && \
    sed -i 's|http://security.ubuntu.com|http://mirrors.aliyun.com|g' /etc/apt/sources.list.d/ubuntu.sources && \
    apt update && \
    apt install -y fonts-noto-cjk sudo wget gnupg libgl1 && \
    apt clean && \
    rm -rf /var/lib/apt/lists/*

ARG WINE_BRANCH="staging"
RUN mkdir -pm755 /etc/apt/keyrings && \
    wget -nv -O /etc/apt/keyrings/winehq-archive.key https://dl.winehq.org/wine-builds/winehq.key && \
    echo "deb [signed-by=/etc/apt/keyrings/winehq-archive.key] https://dl.winehq.org/wine-builds/ubuntu/ $(grep VERSION_CODENAME= /etc/os-release | cut -d= -f2) main" > /etc/apt/sources.list.d/winehq.list && \
    dpkg --add-architecture i386 && \
    apt update && \
    apt install -y --install-recommends winehq-${WINE_BRANCH} && \
    apt install -y --no-install-recommends xvfb libegl1 libegl-mesa0 libglx-mesa0 mesa-vulkan-drivers mesa-utils gosu && \
    rm -rf /var/lib/apt/lists/*

RUN useradd -m docker && \
    echo "docker:docker" | chpasswd && \
    adduser docker sudo && \
    echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers && \
    chmod -R a+X /usr/share/fonts

FROM node:20-bookworm AS server-builder

WORKDIR /build
RUN corepack enable

COPY package.json pnpm-lock.yaml pnpm-workspace.yaml ./
COPY tools/prepare ./tools/prepare
COPY packages/typescript/package.json ./packages/typescript/package.json
RUN pnpm install --frozen-lockfile --filter ./packages/typescript

COPY packages/typescript ./packages/typescript
RUN pnpm --filter ./packages/typescript build

FROM ubuntu:22.04 AS skyline-addon-builder

ARG DEVTOOLS_VERSION
WORKDIR /build
RUN sed -i 's/security.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list && \
    sed -i 's/archive.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list && \
    apt update && \
    apt install -y wget p7zip-full && \
    apt clean && \
    rm -rf /var/lib/apt/lists/*
RUN mkdir -p cache node_modules/skyline-addon && \
    wget -c "https://servicewechat.com/wxa-dev-logic/download_redirect?type=win32_x64&from=mpwiki&download_version=${DEVTOOLS_VERSION}&version_type=1" -O "cache/devtools-${DEVTOOLS_VERSION}.exe" && \
    7z x "cache/devtools-${DEVTOOLS_VERSION}.exe" -aoa -onode_modules/skyline-addon code/package.nw/node_modules/skyline-addon && \
    mv node_modules/skyline-addon/code/package.nw/node_modules/skyline-addon/* node_modules/skyline-addon/ && \
    7z x "cache/devtools-${DEVTOOLS_VERSION}.exe" -aoa -odocumentstart code/package.nw/js/extensions/inject/documentstart/index.js && \
    mv documentstart/code/package.nw/js/extensions/inject/documentstart/index.js documentstart/ && \
    rm -rf node_modules/skyline-addon/code documentstart/code cache

FROM ubuntu:22.04 AS source

ARG APP_ROOT="packages/nodejs"
WORKDIR /workspace
ADD --chmod=644 https://github.com/msojocs/skyline-shared-memory/releases/download/v1.0.4/skyline-sharedMemory-win32-x86_64-v1.0.4.node nwjs/package.nw/node_modules/sharedMemory/sharedMemory.node
RUN sed -i 's/security.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list && \
    sed -i 's/archive.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list && \
    apt update && apt install -y wget unzip && \
    mkdir -p cache && \
    wget -c "https://dl.nwjs.io/v0.54.1/nwjs-sdk-v0.54.1-win-x64.zip" -O "cache/nwjs-sdk-v0.54.1-win-x64.zip" && \
    unzip "cache/nwjs-sdk-v0.54.1-win-x64.zip" && \
    mv nwjs-*-win-x64/* nwjs && \
    rm -rf cache && \
    chmod -R a+X nwjs
COPY --from=server-builder /build/packages/nwjs/server.js ./
COPY packages/nwjs nwjs/package.nw
COPY --from=skyline-addon-builder /build/node_modules/skyline-addon nwjs/package.nw/node_modules/skyline-addon

FROM runtime-base AS runtime
WORKDIR /workspace
ADD --chmod=644 https://github.com/msojocs/wine-emoji-fix/releases/download/dwrite-v1.0.0/dwrite.dll /opt/wine-staging/lib/wine/x86_64-windows/dwrite.dll
COPY --from=source /workspace/nwjs /workspace
COPY packages/nwjs/node_modules/skyline-server /workspace/package.nw/node_modules/
COPY tools/xvfb-startup.sh xvfb-startup.sh

EXPOSE 9222
CMD ["/workspace/xvfb-startup.sh"]
