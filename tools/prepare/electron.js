const { execSync } = require('child_process');
const path = require('path');

if (process.platform == 'win32') {
    const p = path.resolve(__dirname, 'electron.ps1');
    execSync(`powershell ${p}`, { stdio: 'inherit' });
}
else if(process.platform == 'linux') {
    const p = path.resolve(__dirname, 'electron.sh');
    execSync(`bash ${p}`, { stdio: 'inherit' });
}
