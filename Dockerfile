FROM ubuntu:latest AS runtime-base

RUN apt update \
    && apt install -y fonts-noto-cjk fonts-noto-color-emoji fonts-symbola sudo wget gnupg libgl1 \
    && apt clean \
    && rm -rf /var/lib/apt/lists/*

ARG WINE_BRANCH="stable"
RUN wget -nv -O- https://dl.winehq.org/wine-builds/winehq.key | APT_KEY_DONT_WARN_ON_DANGEROUS_USAGE=1 apt-key add - \
    && echo "deb https://dl.winehq.org/wine-builds/ubuntu/ $(grep VERSION_CODENAME= /etc/os-release | cut -d= -f2) main" >> /etc/apt/sources.list \
    && dpkg --add-architecture i386 \
    && apt-get update \
    && DEBIAN_FRONTEND="noninteractive" apt-get install -y --install-recommends winehq-${WINE_BRANCH} \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m docker \
    && echo "docker:docker" | chpasswd \
    && adduser docker sudo \
    && echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

FROM node:20-bookworm AS server-builder

WORKDIR /build
RUN corepack enable

COPY package.json pnpm-lock.yaml pnpm-workspace.yaml ./
COPY tools/prepare ./tools/prepare
COPY packages/typescript/package.json ./packages/typescript/package.json
RUN pnpm install --frozen-lockfile --filter ./packages/typescript

COPY packages/typescript ./packages/typescript
RUN pnpm --filter ./packages/typescript build

FROM ubuntu:latest AS skyline-addon-builder

ARG DEVTOOLS_VERSION="2012510280"
WORKDIR /build
RUN apt update \
    && apt install -y wget p7zip-full \
    && apt clean \
    && rm -rf /var/lib/apt/lists/*
RUN mkdir -p cache node_modules/skyline-addon \
    && wget -c "https://servicewechat.com/wxa-dev-logic/download_redirect?type=win32_x64&from=mpwiki&download_version=${DEVTOOLS_VERSION}&version_type=1" -O "cache/devtools-${DEVTOOLS_VERSION}.exe" \
    && 7z x "cache/devtools-${DEVTOOLS_VERSION}.exe" -aoa -onode_modules/skyline-addon code/package.nw/node_modules/skyline-addon \
    && mv node_modules/skyline-addon/code/package.nw/node_modules/skyline-addon/* node_modules/skyline-addon/ \
    && rm -rf node_modules/skyline-addon/code cache

FROM ubuntu:latest AS source

ARG APP_ROOT="packages/nodejs"
WORKDIR /workspace
ADD https://github.com/msojocs/skyline-node/releases/download/v16.4.0-1/node.exe ./
COPY --from=server-builder /build/packages/nodejs/server.js ./
ADD https://github.com/msojocs/skyline-client-server/releases/download/dll/ucrtbased.dll ./
ADD https://github.com/msojocs/skyline-client-server/releases/download/dll/vcruntime140d.dll ./
ADD https://github.com/msojocs/skyline-shared-memory/releases/download/v1.0.3/skyline-sharedMemory-win32-x86_64-v1.0.3.node ./node_modules/sharedMemory/sharedMemory.node
COPY --from=skyline-addon-builder /build/node_modules/skyline-addon ./node_modules/skyline-addon

FROM runtime-base AS runtime

WORKDIR /workspace
ADD https://github.com/msojocs/wine-emoji-fix/releases/download/dwrite-v1.0.0/dwrite.dll /opt/wine-staging/lib/wine/x86_64-windows/dwrite.dll
COPY --from=source /workspace /workspace

EXPOSE 3001
CMD ["wine", "node.exe", "server.js"]
