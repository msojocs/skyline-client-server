
const args = process.argv.slice(2);
const { exit } = require("process");
const pkg = require("../package.json");
const config = require("../config/config.json");


// 16.17.0后可以使用util.parseArgs，目前是16.11.0
const options = {
    '--get-devtools-version': {
        type: 'boolean',
    },
    '--get-image-version': {
        type: 'boolean',
    },
    '--get-shared-memory-version': {
        type: 'boolean',
    },
}
const configArg = {
    arch: process.arch,
}
for (let i = 0; i < args.length; i++) {
    if (options[args[i]]) {
        if (options[args[i]].type === 'string') {
            i++;
            if (i < args.length) {
                if (args[i - 1] === '--arch') {
                    if (args[i] === 'x64' || args[i] === 'loongarch64' || args[i] === 'arm64') {
                        configArg.arch = args[i];
                    } else {
                        console.error(`Invalid value for option --arch: ${args[i]}`);
                        exit(1);
                    }
                }
            } else {
                console.error(`Missing value for option: ${args[i - 1]}`);
                exit(1);
            }
        } else if (options[args[i]].type === 'boolean') {
            configArg[args[i].substring(2)] = true;
        }
    }
}

if (configArg['get-devtools-version']) {
    console.log(pkg.version.split("-")[0].replace(/\./g, ''));
    exit(0);
}
if (configArg['get-image-version']) {
    console.log(pkg.version);
    exit(0);
}
if (configArg['get-shared-memory-version']) {
    console.log(config['shared-memory']);
    exit(0);
}