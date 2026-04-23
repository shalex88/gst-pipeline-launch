# video-player

[![Build and Run](https://github.com/shalex88/gst-example/actions/workflows/build.yaml/badge.svg)](https://github.com/shalex88/gst-example/actions/workflows/build.yaml)

## Install

```bash
# Install gstreamer
sudo apt -y install pkg-config libgstreamer1.0-dev

# Install gstreamer via vcpkg (not working, gstreamer removed from vcpkg)
sudo apt -y install pkg-config bison flex nasm
```

## TODO

- Add monitoring for pipeline freezes, notify user
- Separate PipelineManager to low level gst pipeline and high level PipelineManager
- Notify user on unsupported commands and fails
- Fix inconsistent representation of multiple GstElement references in PipelineElement (e.g., tee vs. mux)
- Move the config dir to the main project, it is not video-player related
- MAIN PROJECT: Add nvmsgconv parser lib sources from /opt/nvidia/deepstream/deepstream-7.0/sources/libs/nvmsgconv to the project
- Support explicit src pad name in Parser and PipelineElement
- Refactor gst logging messages
- Investigate why the CI run fails when running on Ubuntu 24.04 but does work locally
- Add cmake presets
- Add a .deb runtime dependency on gstreamer packages
- Stopping the app doesn't stop the pipeline thread properly, investigate

## Build options

### Override install root

When configuring the project, you can override the default install root (default: `/tmp/project`) by passing `-DMYAPP_INSTALL_ROOT=<path>` to `cmake`. If the path you provide already includes the project name (for example `/opt/deploy/video-player`), that location will be used as-is; otherwise the project name will be appended so the final install location becomes `<root>/<project-name>`.


This project is designed to build consistently in three ways: via the provided script (with Docker), directly in CLion using a Docker toolchain, and natively on your host IDE.

Common notes

- vcpkg is managed automatically by `cmake/EnableVcpkg.cmake` and uses `vcpkg.json.in` to generate `vcpkg.json` at configure time.
- For cross-compilation, vcpkg chainloads the target toolchain using `VCPKG_CHAINLOAD_TOOLCHAIN_FILE`.
- A `CMakePresets.json` is provided with ready-to-use profiles.

### 1) Build with the script (Docker or native)

```bash
cd submodules/orin/video-player
./build.sh
```

Behavior

- If your toolchain `env.sh` sets `USE_DOCKER_BUILD=1`, the build runs inside the NVIDIA cross-compile Docker image using the repo's `toolchains/aarch64-nvidia-linux-gcc` assets.
- Otherwise, a native build is performed in `build-<toolchain>`.

Artifacts are placed under a build directory next to this README. See the script output for the exact path.

### 2) Build in CLion using Docker (cross-compilation)

1. In CLion, open Settings/Preferences > Build, Execution, Deployment > Toolchains and add a Docker toolchain using the NVIDIA cross image you build for `toolchains/aarch64-nvidia-linux-gcc`.
2. Ensure the container has the environment variable `CMAKE_TOOLCHAIN_FILE` set (our Dockerfile sets it to the NVIDIA toolchain). No other special env is required.
3. In Settings > Build, Execution, Deployment > CMake, enable “Load CMake Presets from CMakePresets.json”.
4. Select the preset: `Orin (Docker) - Release [aarch64]` (preset id: `orin-cross-release`).
5. Build from CLion as usual.

Under the hood, this preset sets

- `VCPKG_TARGET_TRIPLET=arm64-linux`
- `VCPKG_CHAINLOAD_TOOLCHAIN_FILE=$env{CMAKE_TOOLCHAIN_FILE}`

### 3) Build natively in the IDE (host machine)

Two native presets are provided:

- `Native - Debug` (`native-debug`)
- `Native - Release` (`native-release`)

Enable CMake Presets in CLion and pick one of the above, or configure CMake manually without presets. vcpkg will bootstrap automatically and fetch packages for your host.

### Future: Yocto SDK

A placeholder preset `yocto-cross-release` exists which expects the environment variable `YOCTO_SDK_TOOLCHAIN_FILE` to point to your Yocto SDK toolchain file. Once your Yocto SDK is ready, set that variable in your environment or CLion profile and pick the Yocto preset.

## Notes on cross configuration

All cross-compilation specifics (sysroot, pkg-config paths, and linker flags) are now set inside the Docker image via a wrapper toolchain file (`/l4t/orin-toolchain.cmake`). The project `CMakeLists.txt` remains agnostic. Native builds are unaffected.

## Known issues

- Enabling branch that forks from disabled branch is unsupported
- Disabling branch that was forked and the fork is still enabled is unsupported
- Registering multiple branches/elements with the same name is unsupported, maybe we should support it?
- Tee with more than one branch is unsupported, this behavior differs from gst-launch, maybe we should support it?
- Running nvmsgconv element (msgconv_config.yml) behaves differently from gst-launch. source: PipelineManager::generateDynamicPadName() FIXME
- Currently we send ack on enable_branch command even if connection fails. The cause is an async nature of the connection function. Think about using GST_STATE_CHANGE_ASYNC
- When one of the elements in a branch is optional, the branch becomes optional even if it's not defined as optional. source: PipelineManager::getOptionalPipelineBranchesNames()
- nvmsgconv depends on nvinfer. If nvinfer is not enabled and nvmsgconv is, the pipeline will fail. Think about how to handle this. Maybe to allow enable/disable groups of elements
- [2026-02-23 13:29:50.560] [video-player-wfov] [error] [CORE] Failed to sync 'myf2f config-path=/opt/project-system/gst-plugins/plugins/gstmyf2f/config/config-nv.yaml (main)' state with parent
[2026-02-23 13:29:50.560] [video-player-wfov] [info] [API] Response: Success
- 'Failed to open  file: pipeline.yaml' should terminate
-