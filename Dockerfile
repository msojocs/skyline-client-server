FROM ubuntu:22.04 AS runtime-base

ENV DEBIAN_FRONTEND=noninteractive
ENV XVFB_RES="1280x720x24"
ENV XVFB_ARGS=""
ENV WINEDLLOVERRIDES="mscoree,mshtml="
# Force Mesa software rasterizer — no GPU in container
ENV LIBGL_ALWAYS_SOFTWARE=1
ENV GALLIUM_DRIVER=llvmpipe

RUN sed -i 's/security.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    apt update && \
    apt install -y fonts-noto-cjk fonts-symbola sudo wget gnupg libgl1 && \
    apt clean && \
    rm -rf /var/lib/apt/lists/*

ARG WINE_BRANCH="staging"
RUN wget -nv -O- https://dl.winehq.org/wine-builds/winehq.key | APT_KEY_DONT_WARN_ON_DANGEROUS_USAGE=1 apt-key add - && \
    echo "deb https://dl.winehq.org/wine-builds/ubuntu/ $(grep VERSION_CODENAME= /etc/os-release | cut -d= -f2) main" >> /etc/apt/sources.list && \
    dpkg --add-architecture i386 && \
    apt update && \
    apt install -y --install-recommends winehq-${WINE_BRANCH} && \
    apt install -y --no-install-recommends xvfb libegl1-mesa libegl-mesa0 libglx-mesa0 mesa-vulkan-drivers mesa-utils gosu && \
    rm -rf /var/lib/apt/lists/*

RUN useradd -m docker && \
    echo "docker:docker" | chpasswd && \
    adduser docker sudo && \
    echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

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

ARG DEVTOOLS_VERSION="2012510280"
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
    rm -rf node_modules/skyline-addon/code cache

FROM ubuntu:22.04 AS source

ARG APP_ROOT="packages/nodejs"
WORKDIR /workspace
RUN sed -i 's/security.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list && \
    sed -i 's/archive.ubuntu.com/mirrors.tuna.tsinghua.edu.cn/g' /etc/apt/sources.list && \
    apt update && apt install -y wget unzip && \
    mkdir -p cache && \
    wget -c "https://dl.nwjs.io/v0.54.1/nwjs-sdk-v0.54.1-win-x64.zip" -O "cache/nwjs-sdk-v0.54.1-win-x64.zip" && \
    unzip "cache/nwjs-sdk-v0.54.1-win-x64.zip" && \
    mv nwjs-*-win-x64 nwjs && \
    rm -rf cache
COPY --from=server-builder /build/packages/nwjs/server.js ./
COPY packages/nwjs nwjs/package.nw
COPY packages/nwjs/node_modules/sharedMemory/sharedMemory.node nwjs/package.nw/node_modules/sharedMemory/sharedMemory.node
COPY --from=skyline-addon-builder /build/node_modules/skyline-addon nwjs/package.nw/node_modules/skyline-addon

FROM runtime-base AS runtime
WORKDIR /workspace
ADD https://github.com/msojocs/wine-emoji-fix/releases/download/dwrite-v1.0.0/dwrite.dll /opt/wine-staging/lib/wine/x86_64-windows/dwrite.dll
RUN chmod 644 /opt/wine-staging/lib/wine/x86_64-windows/dwrite.dll
COPY --from=source /workspace/nwjs /workspace
COPY packages/nwjs/node_modules/skyline-server /workspace/package.nw/node_modules/
COPY packages/nwjs/documentstart/index.js /workspace/package.nw/documentstart/
COPY tools/xvfb-startup.sh xvfb-startup.sh

EXPOSE 9222
CMD ["/workspace/xvfb-startup.sh"]
