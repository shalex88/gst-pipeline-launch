#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BUILD_TYPE=$1
if [ -z "$BUILD_TYPE" ] || [ "$BUILD_TYPE" != "native" ] && [ "$BUILD_TYPE" != "cross" ]; then
    echo "Error: Invalid or missing build type. Use 'native' or 'cross'." >&2
    exit 1
fi

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

# Check if we should use Docker for building
if [ "$USE_DOCKER_BUILD" = "1" ]; then
    RUN_CONTAINER_SCRIPT="$DOCKER_TOOLCHAIN_DIR/run_container.sh"
    if [ ! -f "$RUN_CONTAINER_SCRIPT" ]; then
        echo "Error: run_container.sh not found at $RUN_CONTAINER_SCRIPT"
        exit 1
    fi
    
    # Get the project root (3 levels up from toolchain dir)
    PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
    
    # Create a temporary script to run inside the container
    TEMP_SCRIPT="/tmp/docker_build_${TOOLCHAIN_NAME}_$$.sh"
    cat > "$TEMP_SCRIPT" << EOFSCRIPT
#!/bin/bash
set -e
cd /workspace/submodules/orin/video-service
source /workspace/toolchains/"$TOOLCHAIN_NAME"/env.sh
exec bash build.sh "$BUILD_TYPE"
EOFSCRIPT
    chmod +x "$TEMP_SCRIPT"
    
    # Run build inside Docker container
    # Mount the entire project root so all submodules are accessible
    "$RUN_CONTAINER_SCRIPT" \
        --args "-v $PROJECT_ROOT:/workspace -v $TEMP_SCRIPT:/tmp/docker_build.sh -w /workspace/submodules/orin/video-service" \
        --exec "/tmp/docker_build.sh"
    
    BUILD_EXIT=$?
    rm -f "$TEMP_SCRIPT"
    exit $BUILD_EXIT
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

    if [ "$BUILD_TYPE" == "cross" ]; then
        cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE"
    else
        cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
    fi

    CMAKE_EXIT=$?
    if [ $CMAKE_EXIT -ne 0 ]; then
        echo "CMake configuration failed with exit code $CMAKE_EXIT" >&2
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