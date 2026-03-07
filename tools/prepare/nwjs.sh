#!/bin/bash

# Set script to exit on any error
set -e

# Navigate to the project's root directory regardless of where the script is executed from
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/../.." &> /dev/null && pwd )"

# Create directories if they don't exist
mkdir -p "$PROJECT_ROOT/packages/native/thirds/nwjs"

# Define the URL and download destination
NODE_LIB_URL="https://dl.nwjs.io/v0.54.1/x64/node.lib"
NODE_LIB_DEST="$PROJECT_ROOT/packages/native/thirds/nwjs/node64.lib"

echo "Downloading node.lib from $NODE_LIB_URL to $NODE_LIB_DEST..."

# Download the file using curl
if command -v curl &> /dev/null; then
    curl -L -o "$NODE_LIB_DEST" "$NODE_LIB_URL"
# Fallback to wget if curl is not available
elif command -v wget &> /dev/null; then
    wget -O "$NODE_LIB_DEST" "$NODE_LIB_URL"
else
    echo "Error: Neither curl nor wget is installed. Please install one of them and try again." >&2
    exit 1
fi

# Verify the download
if [ -f "$NODE_LIB_DEST" ]; then
    echo "Download successful. File saved to $NODE_LIB_DEST"
    # Make the file executable (optional, since it's a .lib file)
    chmod 644 "$NODE_LIB_DEST"
else
    echo "Download failed. File not saved."
    exit 1
fi