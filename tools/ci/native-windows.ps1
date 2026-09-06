param($arch, $tag)
$root_dir = Resolve-Path (Join-Path $PSScriptRoot "../../")

cd $root_dir
try
{
    New-Item -ItemType Directory -Force "$root_dir/packages/electron/node_modules/skyline-server" | Out-Null
    cmake --no-warn-unused-cli -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -S"$root_dir/packages/native" -B"$root_dir/build" -G Ninja
    cmake --build "$root_dir/build" --config Release --target server --
    mkdir "$root_dir/tmp/build"
    Write-Host "$root_dir/build"
    mv "$root_dir/packages/electron/node_modules/skyline-server/server.node" "$root_dir/tmp/build/skyline-server-win32-$arch-$tag.node"
}catch{
    exit 1
}
