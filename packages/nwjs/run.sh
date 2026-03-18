#!/bin/bash

wine /home/msojocs/download/nwjs-sdk-v0.54.1-win-x64/nw.exe . --remote-debugging-port=9222 "--load-extension=/home/msojocs/.config/wechat-devtools/WeappPlugin" "--custom-devtools-frontend=/home/msojocs/github/wechat-web-devtools-linux/package.nw/js/ideplugin/inspector"