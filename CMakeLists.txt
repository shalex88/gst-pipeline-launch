set(PROJECT_NAME video-player)
include(cmake/SetVersion.cmake)
include(cmake/EnableVcpkg.cmake)

if(CMAKE_CROSSCOMPILING)
    message(STATUS "Cross compilation")
else()
    message(STATUS "Native compilation")
endif()

cmake_minimum_required(VERSION 3.22)
project(${PROJECT_NAME} LANGUAGES CXX VERSION ${VERSION})
add_compile_definitions(APP_NAME="${PROJECT_NAME}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCE_DIR "src")
set(CONFIG_DIR "config")

find_package(PkgConfig REQUIRED)
pkg_search_module(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_search_module(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_search_module(GOBJECT REQUIRED IMPORTED_TARGET gobject-2.0)
pkg_search_module(GMODULE REQUIRED IMPORTED_TARGET gmodule-2.0)

find_package(spdlog CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "${SOURCE_DIR}/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    spdlog::spdlog
    yaml-cpp::yaml-cpp
    cxxopts::cxxopts
    PkgConfig::GSTREAMER
    PkgConfig::GLIB
    PkgConfig::GOBJECT
    PkgConfig::GMODULE
)

include(GNUInstallDirs)
if(NOT DEFINED INSTALL_ROOT)
    set(INSTALL_ROOT "/tmp/" CACHE PATH "Root install dir for ${PROJECT_NAME}" FORCE)
endif()
set(CMAKE_INSTALL_PREFIX "${INSTALL_ROOT}" CACHE PATH "Install prefix" FORCE)
set(PROJECT_INSTALL_ROOT "${INSTALL_ROOT}/${PROJECT_NAME}" CACHE PATH "Root install dir for ${PROJECT_NAME}")
set(PROJECT_INSTALL_BINDIR "${PROJECT_INSTALL_ROOT}")
set(PROJECT_INSTALL_LIBDIR "${PROJECT_INSTALL_ROOT}/${CMAKE_INSTALL_LIBDIR}")
set(PROJECT_INSTALL_CONFIGDIR "${PROJECT_INSTALL_ROOT}/${CONFIG_DIR}")
set(CMAKE_INSTALL_RPATH "$ORIGIN/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_BUILD_RPATH  "$ORIGIN/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_SKIP_BUILD_RPATH OFF)
set(CMAKE_SKIP_INSTALL_RPATH OFF)
set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "${CMAKE_INSTALL_RPATH}"
    BUILD_RPATH   "${CMAKE_BUILD_RPATH}"
)

install(TARGETS ${PROJECT_NAME}
    COMPONENT runtime
    RUNTIME DESTINATION "${PROJECT_INSTALL_ROOT}"
)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/config/"
    COMPONENT runtime
    DESTINATION "${PROJECT_INSTALL_CONFIGDIR}"
)

include(cmake/CreatePackage.cmake)