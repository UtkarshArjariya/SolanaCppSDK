#!/usr/bin/env bash
set -e
BUILD_DIR="build"
TEST_FILTER="${1:-}"
if [ ! -d "$BUILD_DIR" ]; then cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug; fi
cmake --build "$BUILD_DIR" --parallel
echo ""
echo "=== Running Tests ==="
if [ -n "$TEST_FILTER" ]; then "$BUILD_DIR/$TEST_FILTER"
else for tb in "$BUILD_DIR"/test_*; do if [ -x "$tb" ]; then echo "--- $(basename $tb) ---"; "$tb"; fi; done; fi
echo "All tests completed."
