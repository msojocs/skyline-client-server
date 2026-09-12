param($arch = "x86_64", $tag = "continuous")
$ErrorActionPreference = "Stop"
$root_dir = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
if ($arch -ne "x86_64") { throw "Unsupported architecture: $arch" }

node "$root_dir/packages/native/build.js" --target x86_64-pc-windows-msvc
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
New-Item -ItemType Directory -Force "$root_dir/tmp/build" | Out-Null
Copy-Item "$root_dir/packages/native/build/x86_64-pc-windows-msvc/render-server.node" "$root_dir/tmp/build/skyline-server-win32-$arch-$tag.node"
Copy-Item "$root_dir/packages/native/build/x86_64-pc-windows-msvc/render-client.node" "$root_dir/tmp/build/skyline-client-win32-$arch-$tag.node"
