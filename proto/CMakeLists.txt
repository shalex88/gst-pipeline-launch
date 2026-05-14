set(PROTO_TARGET proto)

find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# Some cross toolchains provide protobuf/gRPC package metadata that omits the
# absl check-op DSO dependency from generated *.pb.cc objects.
set(ABSL_CHECK_OP_LINK "")
if(TARGET absl::log_internal_check_op)
    set(ABSL_CHECK_OP_LINK absl::log_internal_check_op)
else()
    find_library(ABSL_LOG_INTERNAL_CHECK_OP_LIB NAMES absl_log_internal_check_op)
    if(ABSL_LOG_INTERNAL_CHECK_OP_LIB)
        set(ABSL_CHECK_OP_LINK ${ABSL_LOG_INTERNAL_CHECK_OP_LIB})
    endif()
endif()

find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin
        PATHS "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/tools/grpc"
        NO_DEFAULT_PATH)
if(NOT GRPC_CPP_PLUGIN)
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
endif()

set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(PROTO_OUT_DIR "${CMAKE_BINARY_DIR}/api/proto")
file(MAKE_DIRECTORY "${PROTO_OUT_DIR}")
file(GLOB_RECURSE PROTO_FILES CONFIGURE_DEPENDS "${PROTO_SRC_DIR}/*.proto")

set(PROTO_IMPORT_DIRS "${PROTO_SRC_DIR}")
if(Protobuf_INCLUDE_DIRS)
    list(APPEND PROTO_IMPORT_DIRS ${Protobuf_INCLUDE_DIRS})
endif()
if(Protobuf_INCLUDE_DIR)
    list(APPEND PROTO_IMPORT_DIRS ${Protobuf_INCLUDE_DIR})
endif()
get_target_property(PROTOBUF_TARGET_INCLUDE_DIRS protobuf::libprotobuf INTERFACE_INCLUDE_DIRECTORIES)
if(PROTOBUF_TARGET_INCLUDE_DIRS)
    list(APPEND PROTO_IMPORT_DIRS ${PROTOBUF_TARGET_INCLUDE_DIRS})
endif()
list(REMOVE_DUPLICATES PROTO_IMPORT_DIRS)

add_library(${PROTO_TARGET} ${PROTO_FILES})

target_compile_definitions(${PROTO_TARGET}
        PUBLIC
        GRPC_CALLBACK_API_NONEXPERIMENTAL
)

target_link_libraries(${PROTO_TARGET}
        PUBLIC
        protobuf::libprotobuf
        gRPC::grpc++
        ${ABSL_CHECK_OP_LINK}
        gRPC::grpc++_reflection
)

target_include_directories(${PROTO_TARGET}
        PUBLIC
        ${CMAKE_BINARY_DIR}
        ${Protobuf_INCLUDE_DIRS}
        ${gRPC_INCLUDE_DIRS}
)

protobuf_generate(TARGET ${PROTO_TARGET}
        LANGUAGE cpp
        PROTOC_OUT_DIR ${PROTO_OUT_DIR}
        IMPORT_DIRS ${PROTO_IMPORT_DIRS}
        APPEND_PATH
)

protobuf_generate(TARGET ${PROTO_TARGET}
        LANGUAGE grpc
        GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
        PLUGIN "protoc-gen-grpc=${GRPC_CPP_PLUGIN}"
        PROTOC_OUT_DIR ${PROTO_OUT_DIR}
        IMPORT_DIRS ${PROTO_IMPORT_DIRS}
        APPEND_PATH
)
