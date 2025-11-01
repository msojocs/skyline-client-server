const skylineClient = require("../packages/native/build/skylineClient.node");

const sleep = (ms) => new Promise(resolve => setTimeout(resolve, ms));

(async () => {
    for (let i=0; i<1000;i++) {
        console.info(`--------------->Iteration: ${i}`);
        console.info(global.SkylineDebugInfo);
        await sleep(100)
    }
})()
