# Should be included only from the top level cmake and before project()
include(FetchContent)
FetchContent_Declare(
        vcpkg
        GIT_REPOSITORY https://github.com/Microsoft/vcpkg.git
)
FetchContent_MakeAvailable(vcpkg)

set(ENV{VCPKG_FORCE_SYSTEM_BINARIES} 1)
set(CMAKE_TOOLCHAIN_FILE "${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")