#!/bin/bash
set -ex

root_dir=$(cd `dirname $0`/.. && pwd -P)
# 这个reg文件是拿AI试出来的
wine regedit scripts/wine-font-links.reg
wine node.exe server.js