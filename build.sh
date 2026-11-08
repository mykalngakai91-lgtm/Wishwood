#!/bin/bash
set -e

echo "=== Building Wishwood: Kingdom 3030 ==="
export OO_PS4_TOOLCHAIN="$(pwd)/openorbis"

make clean
make all

echo "Build completed successfully!"
