# Should be included only from the top level cmake and before project()
configure_file(
        ${CMAKE_SOURCE_DIR}/vcpkg.json.in
        ${CMAKE_SOURCE_DIR}/vcpkg.json
        @ONLY
)

include(FetchContent)
FetchContent_Declare(
        vcpkg
        GIT_REPOSITORY https://github.com/Microsoft/vcpkg.git
        GIT_TAG 2025.04.09
)
FetchContent_MakeAvailable(vcpkg)

if(DEFINED VCPKG_CHAINLOAD_TOOLCHAIN_FILE)
    set(CMAKE_CROSSCOMPILING TRUE)
endif()

set(CMAKE_TOOLCHAIN_FILE "${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file" FORCE)