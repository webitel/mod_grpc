cmake_minimum_required(VERSION 3.3)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

project(mod_grpc C CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-gnu-unique -Wreturn-type")
set(third_party_DIR "${CMAKE_SOURCE_DIR}/third_party")

set(USE_GRPC_VERSION "v1.33.2" CACHE STRING "GRPC version")
set(FREESWITCH_INCLUDE_DIR "" CACHE PATH "Location of FreeSWITCH headers")
set(INSTALL_MOD_DIR "/usr/local/freeswitch/mod" CACHE PATH "Location install library")
add_compile_definitions(DEBUG_CURL)

file(MAKE_DIRECTORY ${third_party_DIR})

if (NOT EXISTS "${third_party_DIR}/grpc")
    execute_process(
            COMMAND git clone -b ${USE_GRPC_VERSION} "https://github.com/grpc/grpc.git" grpc
            WORKING_DIRECTORY ${third_party_DIR})

    execute_process(
            COMMAND git submodule update --init --recursive
            WORKING_DIRECTORY ${third_party_DIR}/grpc)
endif()


add_subdirectory(${third_party_DIR}/grpc EXCLUDE_FROM_ALL)

# After using add_subdirectory, we can now use the grpc targets directly from
# this build.
set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_GRPCPP_UNSECURE grpc++_unsecure)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
set(GENERATED_PROTOBUF_PATH "${CMAKE_SOURCE_DIR}/src/generated")

if (NOT EXISTS "${FREESWITCH_INCLUDE_DIR}")
    message( SEND_ERROR "Not exists FREESWITCH_INCLUDE_DIR = ${FREESWITCH_INCLUDE_DIR}" )
endif()

include(proto.cmake)

set(GENERATED_PROTOBUF_FILES ${PROTO_HDRS} ${PROTO_SRCS})

add_library(mod_grpc SHARED src/mod_grpc.h src/mod_grpc.cpp src/Call.cpp src/Call.h src/CallManager.cpp src/CallManager.h src/Cluster.cpp src/Cluster.h
        src/amd_client.cpp src/amd_client.h ${GENERATED_PROTOBUF_FILES}
        src/utils.h)
target_include_directories(mod_grpc PRIVATE ${GENERATED_PROTOBUF_PATH}  ${FREESWITCH_INCLUDE_DIR} )
target_link_libraries(mod_grpc PRIVATE  ${_PROTOBUF_LIBPROTOBUF}  ${_GRPC_GRPCPP_UNSECURE})
#
set_target_properties(mod_grpc PROPERTIES PREFIX "")
set_target_properties(mod_grpc PROPERTIES OUTPUT_NAME "mod_grpc")
#
install(TARGETS mod_grpc DESTINATION ${INSTALL_MOD_DIR})
