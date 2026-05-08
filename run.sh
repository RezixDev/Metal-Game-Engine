#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

echo "🧹 Cleaning previous build..."
rm -rf build

echo "⚙️ Configuring CMake..."
cmake -B build

echo "🔨 Compiling project..."
cmake --build build

echo "🚀 Running MetalEngine..."
cmake --build build --target run
