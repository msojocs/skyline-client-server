FROM scottyhardy/docker-wine:latest

RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo && echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

WORKDIR /workspace
COPY node.exe .
COPY server.js .
COPY *.dll .
COPY node_modules node_modules

EXPOSE 3001
CMD ["wine", "node.exe", "server.js"]