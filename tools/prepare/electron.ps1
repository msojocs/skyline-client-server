# Create directories if they don't exist
$thirdsDir = Join-Path -Path (Split-Path -Path (Split-Path -Path $PSScriptRoot -Parent) -Parent) -ChildPath "packages/native/thirds"
$electronDir = Join-Path -Path $thirdsDir -ChildPath "electron"

if (-not (Test-Path -Path $electronDir)) {
    New-Item -Path $electronDir -ItemType Directory -Force
}

# Define the URL and download destination
$electronVersion = if ($env:ELECTRON_VERSION) { $env:ELECTRON_VERSION } else { "36.6.0" }
$nodeLibUrl = "https://artifacts.electronjs.org/headers/dist/v$electronVersion/win-x64/node.lib"
$nodeLibDest = Join-Path -Path $electronDir -ChildPath "node.lib"

Write-Host "Downloading node.lib from $nodeLibUrl to $nodeLibDest..."

try {
    # Create a WebClient object to download the file
    $webClient = New-Object System.Net.WebClient
    $webClient.DownloadFile($nodeLibUrl, $nodeLibDest)
    
    if (Test-Path -Path $nodeLibDest) {
        Write-Host "Download successful. File saved to $nodeLibDest" -ForegroundColor Green
    } else {
        Write-Host "Download failed. File not saved." -ForegroundColor Red
    }
} catch {
    Write-Error "Error downloading the file: $_" -ForegroundColor Red
}
