cmake_minimum_required(VERSION 3.8)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

project(mod_grpc C CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fno-gnu-unique -Wreturn-type")

option(MOD_BUILD_VERSION "Build version" "")

if (MOD_BUILD_VERSION)
    add_definitions ( -DMOD_BUILD_VERSION=\"${MOD_BUILD_VERSION}\" )
endif()

include(FetchContent)
set(BUILD_TESTING OFF)
# Вимкнути зайві компоненти
set(gRPC_BUILD_CODEGEN OFF CACHE BOOL "Disable gRPC code generation")
set(gRPC_BUILD_CSHARP_PLUGIN OFF CACHE BOOL "Disable C# plugin")
set(gRPC_BUILD_PYTHON_PLUGIN OFF CACHE BOOL "Disable Python plugin")
set(gRPC_BUILD_GRPC_CSHARP_PLUGIN OFF CACHE BOOL "Disable C# gRPC plugin")
set(gRPC_BUILD_GRPC_NODE_PLUGIN OFF CACHE BOOL "Disable Node.js plugin")
set(gRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN OFF CACHE BOOL "Disable Objective-C plugin")
set(gRPC_BUILD_GRPC_RUBY_PLUGIN OFF CACHE BOOL "Disable Ruby plugin")

set(gRPC_BUILD_TESTS OFF CACHE BOOL "Disable gRPC tests")
set(gRPC_BUILD_PERF_TESTS OFF CACHE BOOL "Disable gRPC performance tests")
set(gRPC_ABSL_PROVIDER "package" CACHE STRING "Use system-provided abseil-cpp")
set(gRPC_INSTALL OFF CACHE BOOL "Don't install gRPC")

set(USE_GRPC_VERSION "v1.69.0" CACHE STRING "GRPC version")
set(FREESWITCH_INCLUDE_DIR "/usr/local/freeswitch/include/freeswitch" CACHE PATH "Location of FreeSWITCH headers")
set(INSTALL_MOD_DIR "/usr/local/freeswitch/mod" CACHE PATH "Location install library")
#add_compile_definitions(DEBUG_CURL)
set(third_party_DIR "${CMAKE_SOURCE_DIR}/third_party")

file(MAKE_DIRECTORY ${third_party_DIR})

if (NOT EXISTS "${third_party_DIR}/grpc")
    execute_process(
            COMMAND git clone --recurse-submodules -b ${USE_GRPC_VERSION} --depth 1 --shallow-submodules "https://github.com/grpc/grpc.git" grpc
            WORKING_DIRECTORY ${third_party_DIR})
endif()

add_subdirectory(${third_party_DIR}/grpc EXCLUDE_FROM_ALL)
set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_REFLECTION grpc++_reflection)
set(_GRPC_GRPCPP grpc++)
set(_GRPC_GRPCPP_UNSECURE grpc++_unsecure)

set(WBT_GENERATED_PROTOBUF_PATH "${CMAKE_SOURCE_DIR}/src/generated")
file(GLOB_RECURSE wbt_proto_src ${WBT_GENERATED_PROTOBUF_PATH}/*.cc)

if (NOT EXISTS "${FREESWITCH_INCLUDE_DIR}")
    message( SEND_ERROR "Not exists FREESWITCH_INCLUDE_DIR = ${FREESWITCH_INCLUDE_DIR}" )
endif()

add_library(mod_grpc SHARED src/mod_grpc.h src/mod_grpc.cpp src/Call.cpp src/Call.h src/CallManager.cpp src/CallManager.h src/Cluster.cpp src/Cluster.h
        src/ai_client.cpp src/ai_client.h src/amd_client.cpp src/amd_client.h ${wbt_proto_src}
        src/utils.h src/push_client.cpp src/push_client.h)
target_include_directories(mod_grpc PRIVATE ${WBT_GENERATED_PROTOBUF_PATH}  ${FREESWITCH_INCLUDE_DIR} )
target_link_libraries(mod_grpc PRIVATE  ${_PROTOBUF_LIBPROTOBUF}  ${_GRPC_GRPCPP_UNSECURE})

set_target_properties(mod_grpc PROPERTIES PREFIX "")
set_target_properties(mod_grpc PROPERTIES OUTPUT_NAME "mod_grpc")

install(TARGETS mod_grpc DESTINATION ${INSTALL_MOD_DIR})
