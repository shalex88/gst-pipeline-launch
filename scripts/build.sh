#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

configure_toolchain() {
    if [ -f "$ROOT_DIR/toolchain.yml" ]; then
        TOOLCHAIN_NAME=$(grep "^toolchain:" "$ROOT_DIR/toolchain.yml" | awk '{print $2}')
        if [ -z "$TOOLCHAIN_NAME" ]; then
            echo "Error: Could not parse toolchain name from toolchain.yml" >&2
            exit 1
        fi
    else
        echo "Error: toolchain.yml not found" >&2
        exit 1
    fi

    TOOLCHAIN_DIR="$ROOT_DIR/../../../toolchains/$TOOLCHAIN_NAME"
    TOOLCHAIN_ENV="$TOOLCHAIN_DIR/env.sh"
    if [ ! -f "$TOOLCHAIN_ENV" ]; then
        echo "Error: Toolchain env.sh not found at $TOOLCHAIN_ENV"
        exit 1
    fi

    if  ! source "$TOOLCHAIN_ENV"; then
        exit 1
    fi
}

BUILD_TYPE=$1
if [ -z "$BUILD_TYPE" ] || [ "$BUILD_TYPE" != "native" ] && [ "$BUILD_TYPE" != "cross" ]; then
    echo "Error: Invalid or missing build type. Use 'native' or 'cross'." >&2
    exit 1
fi

BUILD_MODE=$2
if [ -z "$BUILD_MODE" ] || [ "$BUILD_MODE" != "debug" ] && [ "$BUILD_MODE" != "release" ]; then
    echo "Error: Invalid or missing build mode. Use 'debug' or 'release'." >&2
    exit 1
fi

if [ "$BUILD_TYPE" == "cross" ]; then
    configure_toolchain
fi

PRESET="$BUILD_TYPE-$BUILD_MODE"
BUILD_DIR="$ROOT_DIR/build/$PRESET"
LOG_FILE="$BUILD_DIR/build.log"

mkdir -p "$BUILD_DIR"

{
    echo "Build started at $(date)"
    if [ "$BUILD_TYPE" == "cross" ]; then
        echo "Using toolchain: $TOOLCHAIN_NAME"
    else
        echo "Native build"
    fi
    echo "Using CMake preset: $PRESET"
    echo "Build directory: $BUILD_DIR"

    CMAKE_ARGS=(--preset "$PRESET")

    # Add install prefix if INSTALL_ROOT is set
    if [ -n "${INSTALL_ROOT:-}" ]; then
        CMAKE_ARGS+=(-DCMAKE_INSTALL_PREFIX="$INSTALL_ROOT" -DINSTALL_ROOT="$INSTALL_ROOT")
        echo "Install prefix: $INSTALL_ROOT"
    fi

    cd "$ROOT_DIR" || exit 1

    cmake "${CMAKE_ARGS[@]}"

    CMAKE_EXIT=$?
    if [ $CMAKE_EXIT -ne 0 ]; then
        echo "CMake configuration failed with exit code $CMAKE_EXIT" >&2
        echo "Build completed at $(date)"
        exit $CMAKE_EXIT
    fi

    cmake --build --preset "$PRESET" --parallel "$(nproc)"
    BUILD_EXIT=$?
    if [ $BUILD_EXIT -ne 0 ]; then
        echo "Build failed with exit code $BUILD_EXIT" >&2
        echo "Build completed at $(date)"
        exit $BUILD_EXIT
    fi

    cmake --build --preset "$PRESET" --target package
    BUILD_EXIT=$?

    echo "Build log saved to $LOG_FILE"
    echo "Build completed at $(date)"
    exit $BUILD_EXIT
} 2>&1 | tee "$LOG_FILE"

# Capture the exit code from the subshell
exit "${PIPESTATUS[0]}"
