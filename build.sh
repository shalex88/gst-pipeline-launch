#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Read toolchain name from toolchain.yml
if [ -f "$SCRIPT_DIR/toolchain.yml" ]; then
    TOOLCHAIN_NAME=$(grep "^toolchain:" "$SCRIPT_DIR/toolchain.yml" | awk '{print $2}')
    if [ -z "$TOOLCHAIN_NAME" ]; then
        echo "Error: Could not parse toolchain name from toolchain.yml"
        exit 1
    fi
else
    echo "Error: toolchain.yml not found"
    exit 1
fi

# Locate and source the toolchain env.sh file
TOOLCHAIN_DIR="$SCRIPT_DIR/../../../toolchains/$TOOLCHAIN_NAME"
TOOLCHAIN_ENV="$TOOLCHAIN_DIR/env.sh"
if [ ! -f "$TOOLCHAIN_ENV" ]; then
    echo "Error: Toolchain env.sh not found at $TOOLCHAIN_ENV"
    exit 1
fi

source "$TOOLCHAIN_ENV"

BUILD_DIR="build-$TOOLCHAIN_NAME"
LOG_FILE="$BUILD_DIR/build.log"

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"

{
    echo "Build started at $(date)"
    echo "Using toolchain: $TOOLCHAIN_NAME"
    echo "Sourcing environment: $TOOLCHAIN_ENV"
    echo "Build directory: $BUILD_DIR"

    # Use CMAKE_TOOLCHAIN_FILE from environment if set
    if [ -n "$CMAKE_TOOLCHAIN_FILE" ]; then
        echo "Using toolchain file: $CMAKE_TOOLCHAIN_FILE"
        cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE"
    else
        echo "No CMAKE_TOOLCHAIN_FILE set, using environment variables only"
        cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
    fi

    CMAKE_EXIT=$?
    if [ $CMAKE_EXIT -ne 0 ]; then
        echo "CMake configuration failed with exit code $CMAKE_EXIT"
        echo "Build completed at $(date)"
        exit $CMAKE_EXIT
    fi

    cmake --build "$BUILD_DIR" -- -j"$(nproc)"
    BUILD_EXIT=$?

    echo "Build log saved to $SCRIPT_DIR/$LOG_FILE"
    echo "Build completed at $(date)"
    exit $BUILD_EXIT
} 2>&1 | tee "$LOG_FILE"

# Capture the exit code from the subshell
exit ${PIPESTATUS[0]}