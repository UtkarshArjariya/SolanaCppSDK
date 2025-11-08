#!/usr/bin/env bash
set -e
BUILD_TYPE="${1:-Release}"
cmake -B build -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build build --parallel
echo "Build ($BUILD_TYPE) complete."
