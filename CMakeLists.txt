set(PROJECT_NAME gst-pipeline-launch)
include(cmake/SetVersion.cmake)
include(cmake/EnableVcpkg.cmake)

if(DEFINED ENV{CROSS_COMPILE})
    set(CMAKE_CROSSCOMPILING ON)
endif()

if(CMAKE_CROSSCOMPILING)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
    message(STATUS "Cross compilation")
else()
    message(STATUS "Native compilation")
endif()

cmake_minimum_required(VERSION 3.22)
project(${PROJECT_NAME} LANGUAGES CXX VERSION ${VERSION})

add_compile_definitions(APP_NAME="${PROJECT_NAME}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fix pkg-config sysroot for cross-compilation
# The toolchain sets PKG_CONFIG_SYSROOT_DIR to the wrong location
# We need to override it to point to the targetfs
set(ENV{PKG_CONFIG_SYSROOT_DIR} "/l4t/targetfs")
set(ENV{PKG_CONFIG_PATH} "/l4t/targetfs/usr/lib/aarch64-linux-gnu/pkgconfig:/l4t/targetfs/usr/share/pkgconfig")
set(ENV{PKG_CONFIG_LIBDIR} "/l4t/targetfs/usr/lib/aarch64-linux-gnu/pkgconfig:/l4t/targetfs/usr/share/pkgconfig")

find_package(PkgConfig REQUIRED)
pkg_search_module(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_search_module(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_search_module(GOBJECT REQUIRED IMPORTED_TARGET gobject-2.0)
pkg_search_module(GMODULE REQUIRED IMPORTED_TARGET gmodule-2.0)

# Add the targetfs library directories for linking
link_directories(/l4t/targetfs/usr/lib/aarch64-linux-gnu)
link_directories(/l4t/targetfs/lib/aarch64-linux-gnu)
# Add /lib directory for libbz2
link_directories(/l4t/targetfs/lib)

find_package(spdlog CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)

include_directories(
    include
    source
)

file(GLOB_RECURSE sourceFiles CONFIGURE_DEPENDS "source/*.cpp")

add_executable(${PROJECT_NAME} ${sourceFiles})

# Override the toolchain's sysroot for this specific target
# The toolchain adds --sysroot=/l4t/toolchain/.../sysroot but we need /l4t/targetfs
# Using target properties ensures this overrides the toolchain's settings
# Also use --allow-shlib-undefined to ignore missing transitive dependency symbols
set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_FLAGS "-Wl,--sysroot=/l4t/targetfs -Wl,--allow-shlib-undefined"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    spdlog::spdlog
    yaml-cpp::yaml-cpp
    cxxopts::cxxopts
    PkgConfig::GSTREAMER
    PkgConfig::GLIB
    PkgConfig::GOBJECT
    PkgConfig::GMODULE
    # Manually add transitive dependencies that pkg-config doesn't include
    # These must come AFTER the libraries that depend on them
    pcre
    ffi
    unwind
    dw
    elf
    lzma
    z
)