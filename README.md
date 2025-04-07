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

- Add monitoring for pipeline freezes
- Handle (memory:NVMM) caps feature
- Separate PipelineManager to Pipeline and PipelineManager?
- Check refcounting enable-disable element/branch

## Known issues

- Enabling branch that forks from disabled branch is unsupported
- Disabling branch that was forks and the fork is still enabled is unsupported
- Registering multiple branches/elements with the same name is unsupported
- gst_object_unref errors when disabling branches
- Tee with more than one branch is unsupported
- Inconsistent representation of multiple GstElement references in PipelineElement (e.g., tee vs. mux)