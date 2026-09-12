const { spawnSync } = require('node:child_process');
const { copyFileSync, mkdirSync } = require('node:fs');
const path = require('node:path');

function run(command, args, options = {}) {
  const result = spawnSync(command, args, {
    cwd: __dirname,
    encoding: 'utf8',
    maxBuffer: 32 * 1024 * 1024,
    ...options,
  });
  if (result.error) throw result.error;
  if (result.status !== 0) {
    throw new Error(`${command} failed (${result.status ?? result.signal})\n${result.stderr || ''}`);
  }
  return result.stdout;
}

function main() {
  const args = process.argv.slice(2);
  const host = run('rustc', ['-vV']).split('\n').find(line => line.startsWith('host: '))?.slice(6);
  if (!host) throw new Error('Cannot determine the Rust host target');
  let target = host;
  let module = 'all';
  let profile = 'release';
  for (let i = 0; i < args.length; i++) {
    if (args[i] === '--target') target = args[++i];
    else if (args[i] === '--module') module = args[++i];
    else if (args[i] === '--debug') profile = 'debug';
    else throw new Error(`Unknown argument: ${args[i]}`);
  }
  const modules = { client: 'render-client.node', server: 'render-server.node', 'main-client': 'main-client.node' };
  // 'both' 是历史写法，等同 'all'。
  if (!['all', 'both'].includes(module) && !Object.hasOwn(modules, module)) {
    throw new Error(`--module must be ${[...Object.keys(modules), 'all'].join(', ')}`);
  }
  if (!['x86_64-unknown-linux-gnu', 'x86_64-pc-windows-gnu', 'x86_64-pc-windows-msvc'].includes(target)) {
    throw new Error(`Unsupported target: ${target}`);
  }
  const output = path.join(__dirname, 'build', target);
  mkdirSync(output, { recursive: true });
  for (const feature of ['all', 'both'].includes(module) ? Object.keys(modules) : [module]) {
    const cargoArgs = ['build', '--locked', '--target', target, '--no-default-features', '--features', feature,
      '--message-format=json-render-diagnostics'];
    if (profile === 'release') cargoArgs.push('--release');
    const stdout = run('cargo', cargoArgs, { stdio: ['ignore', 'pipe', 'inherit'] });
    const artifacts = stdout.split('\n').filter(Boolean).map(line => JSON.parse(line));
    const artifact = artifacts.findLast(item => item.reason === 'compiler-artifact'
      && item.target.name === 'skyline_native' && item.target.crate_types.includes('cdylib'));
    const library = artifact?.filenames.find(file => /\.(so|dll|dylib)$/.test(file));
    if (!library) throw new Error(`Cargo did not produce a ${feature} library`);
    const name = modules[feature];
    const destination = path.join(output, name);
    copyFileSync(library, destination);
    console.log(destination);
    // main 层客户端由 devtools main 层加载，不落在本仓库的运行时目录。
    if (target === host && feature !== 'main-client') {
      const local = feature === 'client'
        ? path.join(process.env.SKYLINE_DEV_PATH || path.join(__dirname, 'build'), name)
        : path.join(__dirname, '../electron/node_modules/skyline-server', name);
      mkdirSync(path.dirname(local), { recursive: true });
      copyFileSync(destination, local);
    }
  }
}

try { main(); } catch (error) { console.error(error.message); process.exitCode = 1; }
