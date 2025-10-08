console.info('start test.js')
const skyline = require('skyline-addon')
console.info(skyline)
console.info(globalThis.SkylineShell)
try {
    // console.info(globalThis)
    // const shell = new global.SkylineShell()
} catch(e) {
    console.error(e)
}