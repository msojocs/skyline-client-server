const client = require('/home/msojocs/github/wechat-web-devtools-linux/package.nw/node_modules/skyline-addon/build/skyline.node')

async function benchmark() {
    const start = Date.now()
    for (let i = 0; i < 10000; i++) {
        SkylineDebugInfo
    }
    const end = Date.now()
    console.log(`Time taken: ${end - start}ms`)
}

benchmark()