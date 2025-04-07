const skylineClient = require('../build/skylineClient.node');

console.info('1111');
console.info('Using global.SkylineDebugInfo:');
console.info(global.SkylineDebugInfo);
console.info(global.SkylineDebugInfo);
const shell = new global.SkylineShell()
console.info(shell);
shell.setNotifyBootstrapDoneCallback(() => {
    console.info('Bootstrap done');
});