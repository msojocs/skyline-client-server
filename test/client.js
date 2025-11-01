const path = require('path');
const clientNode = process.env['SKYLINE_DEV_PATH']
    ? `${process.env['SKYLINE_DEV_PATH']}/skyline.node` 
    : path.resolve(__dirname, "../packages/native/build/skyline.node")

const skylineClient = require(clientNode);

for (let i = 0; i < 100; i++) {
    console.info(global.SkylineDebugInfo);
}