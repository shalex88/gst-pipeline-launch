cmake_minimum_required(VERSION 3.22)

#include(cmake/vcpkg.cmake) # Currently gstreamer via vcpkg is not supported on linux

project(gst-example VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W3)
else()
    message(WARNING "Unknown Compiler ${CMAKE_CXX_COMPILER_ID}")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

include_directories(
    include
    ${GLIB_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
)

link_directories(
    ${GLIB_LIBRARY_DIRS}
    ${GSTREAMER_LIBRARY_DIRS}
)

file(GLOB_RECURSE sourceFiles CONFIGURE_DEPENDS "source/*.cpp")

add_executable(${PROJECT_NAME} ${sourceFiles})

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${GSTREAMER_LIBRARIES}
)