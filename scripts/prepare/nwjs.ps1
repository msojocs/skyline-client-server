# Create directories if they don't exist
$thirdsDir = Join-Path -Path (Split-Path -Path (Split-Path -Path $PSScriptRoot -Parent) -Parent) -ChildPath "packages/native/thirds"
$nwjsDir = Join-Path -Path $thirdsDir -ChildPath "nwjs"

if (-not (Test-Path -Path $nwjsDir)) {
    New-Item -Path $nwjsDir -ItemType Directory -Force
}

# Define the URL and download destination
$nodeLibUrl = "https://dl.nwjs.io/v0.54.1/x64/node.lib"
$nodeLibDest = Join-Path -Path $nwjsDir -ChildPath "node64.lib"

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