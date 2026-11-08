#!/bin/bash
set -e

echo "=== Building Wishwood: Kingdom 3030 ==="

# Set OpenOrbis toolchain paths
export OO_PS4_TOOLCHAIN="$(pwd)/openorbis"
export PATH="$OO_PS4_TOOLCHAIN/bin:$PATH"

# Clean previous builds
make clean

# Build eboot.bin using orbis-clang
make all

echo "Build completed successfully!"
