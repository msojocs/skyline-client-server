const { execSync } = require('child_process');
const path = require('path');

if (process.platform == 'win32') {
    const p = path.resolve(__dirname, 'nwjs.ps1');
    execSync(`powershell ${p}`, { stdio: 'inherit' });
}
else if(process.platform == 'linux') {
    const p = path.resolve(__dirname, 'nwjs.sh');
    execSync(`bash ${p}`, { stdio: 'inherit' });
}