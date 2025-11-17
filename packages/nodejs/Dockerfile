FROM ubuntu:latest

RUN apt update \
    && apt install -y fonts-noto-cjk sudo wget gnupg libgl1 \
    && apt clean && rm -rf /var/lib/apt/lists/*

# Install wine
ARG WINE_BRANCH="stable"
RUN wget -nv -O- https://dl.winehq.org/wine-builds/winehq.key | APT_KEY_DONT_WARN_ON_DANGEROUS_USAGE=1 apt-key add - \
    && echo "deb https://dl.winehq.org/wine-builds/ubuntu/ $(grep VERSION_CODENAME= /etc/os-release | cut -d= -f2) main" >> /etc/apt/sources.list \
    && dpkg --add-architecture i386 \
    && apt-get update \
    && DEBIAN_FRONTEND="noninteractive" apt-get install -y --install-recommends winehq-${WINE_BRANCH} \
    && rm -rf /var/lib/apt/lists/*
RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo && echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

WORKDIR /workspace
COPY node.exe .
COPY server.js .
COPY *.dll .
COPY node_modules node_modules
COPY scripts scripts

EXPOSE 3001
CMD ["scripts/start-server.sh"]