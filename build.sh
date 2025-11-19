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

# Check if we should use Docker for building
if [ "$USE_DOCKER_BUILD" = "1" ]; then
    echo "Docker build mode detected"
    echo "Building Docker image and running build inside container..."
    
    # Find the Dockerfile location
    RUN_CONTAINER_SCRIPT="$DOCKER_TOOLCHAIN_DIR/run_container.sh"
    if [ ! -f "$RUN_CONTAINER_SCRIPT" ]; then
        echo "Error: run_container.sh not found at $RUN_CONTAINER_SCRIPT"
        exit 1
    fi
    
    # Get the project root (3 levels up from toolchain dir)
    PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
    
    # Create a temporary script to run inside the container
    TEMP_SCRIPT="/tmp/docker_build_${TOOLCHAIN_NAME}_$$.sh"
    cat > "$TEMP_SCRIPT" << 'EOFSCRIPT'
#!/bin/bash
set -e
cd /workspace/submodules/orin/video-service
source /workspace/toolchains/aarch64-nvidia-linux-gcc/env.sh
exec bash build.sh
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

BUILD_DIR="build-$TOOLCHAIN_NAME"
LOG_FILE="$BUILD_DIR/build.log"

# Create build directory if it doesn't exist
# Clean CMake cache if it exists to ensure fresh configuration
if [ -d "$BUILD_DIR" ]; then
    rm -f "$BUILD_DIR/CMakeCache.txt"
    rm -rf "$BUILD_DIR/CMakeFiles"
fi
mkdir -p "$BUILD_DIR"

{
    echo "Build started at $(date)"
    echo "Using toolchain: $TOOLCHAIN_NAME"
    echo "Sourcing environment: $TOOLCHAIN_ENV"
    echo "Build directory: $BUILD_DIR"

    # For cross-compilation with vcpkg, we need to use vcpkg's toolchain
    # and chainload the cross-compilation toolchain via VCPKG_CHAINLOAD_TOOLCHAIN_FILE
    if [ -n "$CMAKE_TOOLCHAIN_FILE" ]; then
        cmake -S . -B "$BUILD_DIR" \
            -DCMAKE_BUILD_TYPE=Release \
            -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE"
    else
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