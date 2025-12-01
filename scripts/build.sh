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

if [ "$BUILD_TYPE" == "cross" ]; then
    configure_toolchain
    # Check if we should use Docker for building
    if [ "$USE_DOCKER_BUILD" = "1" ]; then
        RUN_CONTAINER_SCRIPT="$DOCKER_TOOLCHAIN_DIR/run_container.sh"
        if [ ! -f "$RUN_CONTAINER_SCRIPT" ]; then
            echo "Error: run_container.sh not found at $RUN_CONTAINER_SCRIPT"
            exit 1
        fi

        PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"

        # Create a temporary script to run inside the container
        TEMP_SCRIPT="/tmp/docker_build_${BUILD_TYPE}_$$.sh"
        cat > "$TEMP_SCRIPT" << EOFSCRIPT
#!/bin/bash -e
cd /workspace/submodules/orin/video-player
source /workspace/toolchains/"$TOOLCHAIN_NAME"/env.sh
# Propagate INSTALL_ROOT if set
if [ -n "\${INSTALL_ROOT:-}" ]; then
    export INSTALL_ROOT
fi
exec bash scripts/build.sh "$BUILD_TYPE"
EOFSCRIPT
        chmod +x "$TEMP_SCRIPT"

        # Run build inside Docker container
        # Mount the entire project root so all submodules are accessible
        # Propagate INSTALL_ROOT into the container if set
        DOCKER_ARGS="-v $PROJECT_ROOT:/workspace -v $TEMP_SCRIPT:/tmp/docker_build.sh -w /workspace/submodules/orin/video-player"
        if [ -n "${INSTALL_ROOT:-}" ]; then
            DOCKER_ARGS="$DOCKER_ARGS -e INSTALL_ROOT=$INSTALL_ROOT"
            echo "Propagating INSTALL_ROOT to container: $INSTALL_ROOT"
        fi
        "$RUN_CONTAINER_SCRIPT" \
            --args "$DOCKER_ARGS" \
            --exec "/tmp/docker_build.sh"

        BUILD_EXIT=$?
        rm -f "$TEMP_SCRIPT"
        exit $BUILD_EXIT
    fi
fi

# Use this to prevent installing gstreamer via vcpkg
if [ "$BUILD_TYPE" == "native" ]; then
    sudo apt -y install pkg-config libgstreamer1.0-dev
fi

BUILD_DIR="build-$BUILD_TYPE"
LOG_FILE="$BUILD_DIR/build.log"

mkdir -p "$BUILD_DIR"

{
    echo "Build started at $(date)"
    if [ "$BUILD_TYPE" == "cross" ]; then
        echo "Using toolchain: $TOOLCHAIN_NAME"
    else
        echo "Native build"
    fi
    echo "Build directory: $BUILD_DIR"

    CMAKE_ARGS=(-G Ninja -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release)
    
    # Add install prefix if INSTALL_ROOT is set
    if [ -n "${INSTALL_ROOT:-}" ]; then
        CMAKE_ARGS+=(-DCMAKE_INSTALL_PREFIX="$INSTALL_ROOT" -DINSTALL_ROOT="$INSTALL_ROOT")
        echo "Install prefix: $INSTALL_ROOT"
    fi

    if [ "$BUILD_TYPE" == "cross" ]; then
        CMAKE_ARGS+=(-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE" -DVCPKG_TARGET_TRIPLET="$VCPKG_TARGET_TRIPLET")
        cmake "${CMAKE_ARGS[@]}"
    else
        cmake "${CMAKE_ARGS[@]}"
    fi

    CMAKE_EXIT=$?
    if [ $CMAKE_EXIT -ne 0 ]; then
        echo "CMake configuration failed with exit code $CMAKE_EXIT" >&2
        echo "Build completed at $(date)"
        exit $CMAKE_EXIT
    fi

    cmake --build "$BUILD_DIR" -- -j"$(nproc)"
    BUILD_EXIT=$?
    if [ $BUILD_EXIT -ne 0 ]; then
        echo "Build failed with exit code $BUILD_EXIT" >&2
        echo "Build completed at $(date)"
        exit $BUILD_EXIT
    fi

    cmake --build "$BUILD_DIR" --target package
    BUILD_EXIT=$?

    echo "Build log saved to $PROJECT_ROOT/$LOG_FILE"
    echo "Build completed at $(date)"
    exit $BUILD_EXIT
} 2>&1 | tee "$LOG_FILE"

# Capture the exit code from the subshell
exit "${PIPESTATUS[0]}"