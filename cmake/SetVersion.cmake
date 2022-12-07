if(NOT DEFINED VERSION)
    set(VERSION "0.0.0" CACHE STRING "Version number")
endif()

# Parse major, minor, and patch version from VERSION
string(REGEX REPLACE "^([0-9]+)\\..*" "\\1" VERSION_MAJOR ${VERSION})
string(REGEX REPLACE "^[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR ${VERSION})
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_PATCH ${VERSION})

# Add the compile definitions
add_compile_definitions(
        APP_VERSION_MAJOR=${VERSION_MAJOR}
        APP_VERSION_MINOR=${VERSION_MINOR}
        APP_VERSION_PATCH=${VERSION_PATCH}
)