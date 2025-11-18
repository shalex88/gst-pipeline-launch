# Get the latest git tag
execute_process(
        COMMAND git describe --tags
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE GIT_DESCRIBE_RESULT
)

# If VERSION is not defined, set it to the latest git tag or "0.0.0" if no tag is found
if(NOT DEFINED VERSION)
    if(GIT_TAG)
        string(REGEX REPLACE "v" "" GIT_TAG ${GIT_TAG})
        if(GIT_TAG MATCHES "-")
            set(VERSION_DIRTY "-dirty")
        else()
            set(VERSION_DIRTY "")
        endif()
        string(REGEX REPLACE "^v" "" GIT_TAG ${GIT_TAG})
        string(REGEX REPLACE "-.*" "" GIT_TAG ${GIT_TAG})
        set(VERSION ${GIT_TAG} CACHE STRING "Version number")
    else()
        set(VERSION "0.0.0" CACHE STRING "Version number")
        set(VERSION_DIRTY "")
    endif()
endif()

# Parse major, minor, and patch version from VERSION
string(REGEX REPLACE "^([0-9]+)\\..*" "\\1" VERSION_MAJOR ${VERSION})
string(REGEX REPLACE "^[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR ${VERSION})
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_PATCH ${VERSION})

message(STATUS "Version: v${VERSION}${VERSION_DIRTY}")

# Add the compile definitions
add_compile_definitions(
        APP_VERSION_MAJOR=${VERSION_MAJOR}
        APP_VERSION_MINOR=${VERSION_MINOR}
        APP_VERSION_PATCH=${VERSION_PATCH}
        APP_VERSION_DIRTY="${VERSION_DIRTY}"
)