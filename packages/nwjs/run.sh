#!/bin/bash

wine $HOME/download/nwjs-sdk-v0.54.1-win-x64/nw.exe . --remote-debugging-port=9222 "--load-extension=$HOME/.config/wechat-devtools/WeappPlugin" "--custom-devtools-frontend=$HOME/github/wechat-web-devtools-linux/package.nw/js/ideplugin/inspector"