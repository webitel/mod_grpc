cmake_minimum_required(VERSION 3.8)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(mod_grpc C CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fno-gnu-unique -Wreturn-type")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -s")

option(MOD_BUILD_VERSION "Build version" "")

if (MOD_BUILD_VERSION)
    add_definitions(-DMOD_BUILD_VERSION=\"${MOD_BUILD_VERSION}\")
endif ()

set(FREESWITCH_INCLUDE_DIR "/usr/local/freeswitch/include/freeswitch" CACHE PATH "Location of FreeSWITCH headers")
set(INSTALL_MOD_DIR "/usr/local/freeswitch/mod" CACHE PATH "Location install library")

# Set custom paths for locally installed protobuf/grpc
list(PREPEND CMAKE_PREFIX_PATH "/usr/local")
set(ENV{PKG_CONFIG_PATH} "/usr/local/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")

# Find pre-installed gRPC and Protobuf
find_package(PkgConfig REQUIRED)

# Check protobuf version first
find_program(PROTOC_EXECUTABLE
        NAMES protoc
        PATHS /usr/local/bin /usr/bin
        REQUIRED
)

execute_process(
        COMMAND ${PROTOC_EXECUTABLE} --version
        OUTPUT_VARIABLE PROTOC_VERSION_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Found protoc version: ${PROTOC_VERSION_OUTPUT}")

# Try to find gRPC using pkg-config first
pkg_check_modules(GRPC grpc++)
pkg_check_modules(PROTOBUF protobuf)

# Alternative method using find_package
if(NOT GRPC_FOUND OR NOT PROTOBUF_FOUND)
    message(STATUS "pkg-config failed, trying find_package...")
    find_package(Protobuf REQUIRED)
    find_package(gRPC CONFIG REQUIRED)

    # Set variables for compatibility
    set(GRPC_LIBRARIES gRPC::grpc++)
    set(PROTOBUF_LIBRARIES protobuf::libprotobuf)
    set(GRPC_INCLUDE_DIRS "")
    set(PROTOBUF_INCLUDE_DIRS "")
endif()

# Find grpc_cpp_plugin
find_program(GRPC_CPP_PLUGIN_EXECUTABLE
        NAMES grpc_cpp_plugin
        PATHS /usr/local/bin /usr/bin
        REQUIRED
)

message(STATUS "Using protoc: ${PROTOC_EXECUTABLE}")
message(STATUS "Using grpc_cpp_plugin: ${GRPC_CPP_PLUGIN_EXECUTABLE}")
message(STATUS "gRPC libraries: ${GRPC_LIBRARIES}")
message(STATUS "Protobuf libraries: ${PROTOBUF_LIBRARIES}")

set(WBT_GENERATED_PROTOBUF_PATH "${CMAKE_SOURCE_DIR}/src/generated")
file(GLOB_RECURSE wbt_proto_src ${WBT_GENERATED_PROTOBUF_PATH}/*.cc)

if (NOT EXISTS "${FREESWITCH_INCLUDE_DIR}")
    message(SEND_ERROR "Not exists FREESWITCH_INCLUDE_DIR = ${FREESWITCH_INCLUDE_DIR}")
endif ()

add_library(mod_grpc SHARED
        src/mod_grpc.h
        src/mod_grpc.cpp
        src/Call.h
        src/CallManager.cpp
        src/CallManager.h
        src/Cluster.cpp
        src/Cluster.h
        src/voice_bot_client.h
        src/amd_client.cpp
        src/amd_client.h
        ${wbt_proto_src}
        src/utils.h
        src/push_client.cpp
        src/push_client.h
)

target_include_directories(mod_grpc PRIVATE
        ${WBT_GENERATED_PROTOBUF_PATH}
        ${FREESWITCH_INCLUDE_DIR}
        ${GRPC_INCLUDE_DIRS}
        ${PROTOBUF_INCLUDE_DIRS}
)

target_link_libraries(mod_grpc PRIVATE
        ${GRPC_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
)

# Add compiler flags if needed
if(GRPC_CFLAGS_OTHER)
    target_compile_options(mod_grpc PRIVATE ${GRPC_CFLAGS_OTHER})
endif()

if(PROTOBUF_CFLAGS_OTHER)
    target_compile_options(mod_grpc PRIVATE ${PROTOBUF_CFLAGS_OTHER})
endif()

set_target_properties(mod_grpc PROPERTIES PREFIX "")
set_target_properties(mod_grpc PROPERTIES OUTPUT_NAME "mod_grpc")

install(TARGETS mod_grpc DESTINATION ${INSTALL_MOD_DIR})