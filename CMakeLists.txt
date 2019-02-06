cmake_minimum_required(VERSION 3.3)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

project(mod_grpc C CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-gnu-unique")

find_package(PkgConfig REQUIRED)


add_subdirectory(grpc EXCLUDE_FROM_ALL)
message(STATUS "Using gRPC via add_subdirectory.")

# After using add_subdirectory, we can now use the grpc targets directly from
# this build.
set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_GRPCPP_UNSECURE grpc++_unsecure)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)

pkg_check_modules(FREESWITCH REQUIRED freeswitch)
set(FS_MOD_DIR /usr/local/freeswitch/mod/)

set(PROTO_PATH "${CMAKE_SOURCE_DIR}/src/protos")
set(FS_PROTO "${PROTO_PATH}/fs.proto")
set(GENERATED_PROTOBUF_PATH "${CMAKE_SOURCE_DIR}/src/generated")
file(MAKE_DIRECTORY ${GENERATED_PROTOBUF_PATH})

set(FS_PB_CPP_FILE "${GENERATED_PROTOBUF_PATH}/fs.pb.cc")
set(FS_PB_H_FILE "${GENERATED_PROTOBUF_PATH}/fs.pb.h")
set(FS_GRPC_PB_CPP_FILE "${GENERATED_PROTOBUF_PATH}/fs.grpc.pb.cc")
set(FS_GRPC_PB_H_FILE "${GENERATED_PROTOBUF_PATH}/fs.grpc.pb.h")

add_custom_command(
        OUTPUT "${FS_PB_CPP_FILE}" "${FS_PB_H_FILE}" "${FS_GRPC_PB_CPP_FILE}" "${FS_GRPC_PB_H_FILE}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${GENERATED_PROTOBUF_PATH}"
        --cpp_out "${GENERATED_PROTOBUF_PATH}"
        -I "${PROTO_PATH}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${FS_PROTO}"
        DEPENDS "${FS_PROTO}")

set(GENERATED_PROTOBUF_FILES ${FS_PB_H_FILE} ${FS_PB_CPP_FILE}
        ${FS_GRPC_PB_H_FILE} ${FS_GRPC_P_CPP_FILE})


add_library(mod_grpc SHARED src/mod_grpc.h src/mod_grpc.cpp ${FS_PB_CPP_FILE} ${FS_PB_H_FILE} ${FS_GRPC_PB_CPP_FILE} ${FS_GRPC_PB_H_FILE})
target_include_directories(mod_grpc PRIVATE ${GENERATED_PROTOBUF_PATH}  ${FREESWITCH_INCLUDE_DIRS} )
target_link_libraries(mod_grpc PRIVATE  ${_PROTOBUF_LIBPROTOBUF}  ${_GRPC_GRPCPP_UNSECURE}
        -pthread)

set_target_properties(mod_grpc PROPERTIES PREFIX "")
set_target_properties(mod_grpc PROPERTIES OUTPUT_NAME "mod_grpc")

install(TARGETS mod_grpc DESTINATION ${FS_MOD_DIR})