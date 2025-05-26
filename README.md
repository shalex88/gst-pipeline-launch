# gst-pipeline-launch

[![Build and Run](https://github.com/shalex88/gst-example/actions/workflows/build.yaml/badge.svg)](https://github.com/shalex88/gst-example/actions/workflows/build.yaml)

# Install

```bash
# Install gstreamer
sudo apt -y install pkg-config libgstreamer1.0-dev

# Install gstreamer via vcpkg (not working, gst_element_factory_make() returns NULL)
sudo apt -y install pkg-config bison flex nasm
```

## TODO

- Add monitoring for pipeline freezes, notify user
- Separate PipelineManager to low level gst pipeline and high level PipelineManager
- Notify user on unsupported commands and fails
- Fix inconsistent representation of multiple GstElement references in PipelineElement (e.g., tee vs. mux)
- Move the resources dir to the main project, it is not gst-pipeline-launcher related
- MAIN PROJECT: Add nvmsgconv parser lib sources from /opt/nvidia/deepstream/deepstream-7.0/sources/libs/nvmsgconv to the project
- Clean up elements that were already created and linked On branch-connection failure
- Support explicit src pad name in Parser and PipelineElement
- Refactor gst logging messages

## Known issues

- Enabling branch that forks from disabled branch is unsupported
- Disabling branch that was forked and the fork is still enabled is unsupported
- Registering multiple branches/elements with the same name is unsupported, maybe we should support it?
- Tee with more than one branch is unsupported, this behavior differs from gst-launch, maybe we should support it?
- Running nvmsgconv element (msgconv_config.yml) behaves differently from gst-launch. source: PipelineManager::generateDynamicPadName() FIXME
- Currently we send ack on enable_branch command even if connection fails. The cause is an async nature of the connection function.
- Failed to dynamic link tee to the first element when the source is live GST_PAD_LINK_NOFORMAT
- When one of the elements in a branch is optional, the branch becomes optional even if it's not defined as optional. source: PipelineManager::getOptionalPipelineBranchesNames()
- When a branch connection fails, any elements of the branch that have already been initialized and linked are not cleaned up.