#!/bin/bash
set -e
echo "=== Building Wishwood: Kingdom 3030 ==="
make clean
make all
echo "=== Build Complete ==="
ls -la WISH30300.pkg
