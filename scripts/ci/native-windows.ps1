param($arch, $tag)
$root_dir = Resolve-Path (Join-Path $PSScriptRoot "../../")

cd $root_dir
try
{
    cmake --no-warn-unused-cli -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -S"$root_dir/packages/native" -B"$root_dir/build" -G Ninja
    cmake --build "$root_dir/build" --config Release --target skylineServer --
    mkdir "$root_dir/tmp/build"
    Write-Host "$root_dir/build"
    mv "$root_dir/packages/nodejs/node_modules/skyline-server/skylineServer.node" "$root_dir/tmp/build/skyline-skylineServer-win32-$arch-$tag.node"
}catch{
    exit 1
}