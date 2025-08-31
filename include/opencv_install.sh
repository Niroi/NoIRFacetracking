#!/usr/bin/env bash
set -euo pipefail

# =========================================
# Config
OPENCV_VERSION="4.10.0"
INSTALL_PREFIX="$(pwd)/include/opencv"
# =========================================

# Create temp dir for building
TMPDIR="$(mktemp -d)"
echo "🔧 Using temporary build directory: $TMPDIR"

# Ensure cleanup on exit (success or fail)
cleanup() {
    echo "🧹 Cleaning up temporary directory..."
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

# Clone OpenCV sources into tmpdir
git clone --branch "$OPENCV_VERSION" --depth 1 https://github.com/opencv/opencv.git "$TMPDIR/opencv"
git clone --branch "$OPENCV_VERSION" --depth 1 https://github.com/opencv/opencv_contrib.git "$TMPDIR/opencv_contrib"

# Configure
cmake -S "$TMPDIR/opencv" -B "$TMPDIR/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    -DOPENCV_EXTRA_MODULES_PATH="$TMPDIR/opencv_contrib/modules" \
    -DBUILD_TESTS=OFF \
    -DBUILD_PERF_TESTS=OFF \
    -DBUILD_EXAMPLES=OFF

# Build + install
cmake --build "$TMPDIR/build" -j"$(nproc)"
cmake --install "$TMPDIR/build"

echo "✅ OpenCV $OPENCV_VERSION installed to $INSTALL_PREFIX"
