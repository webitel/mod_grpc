
file(MAKE_DIRECTORY ${WBT_GENERATED_PROTOBUF_PATH})
set(WBT_PROTO_PATH "${CMAKE_SOURCE_DIR}/src/protos")

file(GLOB_RECURSE wbt_proto_files ABSOLUTE ${WBT_PROTO_PATH}/*.proto)

# Generate Protobuf cpp sources
set(WBT_PROTO_HDRS)
set(WBT_PROTO_SRCS)
foreach(WBT_PROTO_FILE IN LISTS wbt_proto_files)
    #message(STATUS "protoc proto(cc): ${WBT_PROTO_FILE}")
#    get_filename_component(PROTO_DIR ${WBT_PROTO_FILE} DIRECTORY)
    get_filename_component(PROTO_NAME ${WBT_PROTO_FILE} NAME_WE)
    set(WBT_PROTO_HDR ${WBT_GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.pb.h)
    set(WBT_PROTO_SRC ${WBT_GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.pb.cc)
    set(WBT_PROTO_GRPC_HDR ${WBT_GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.grpc.pb.h)
    set(WBT_PROTO_GRPC_SRC ${WBT_GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.grpc.pb.cc)
    message(STATUS "protoc hdr: ${WBT_PROTO_HDR}")
    message(STATUS "protoc src: ${WBT_PROTO_SRC}")
    add_custom_command(
            OUTPUT ${WBT_PROTO_SRC} ${WBT_PROTO_HDR} ${WBT_PROTO_GRPC_SRC} ${WBT_PROTO_GRPC_HDR}
            COMMAND ${_PROTOBUF_PROTOC}
            ARGS --grpc_out "${WBT_GENERATED_PROTOBUF_PATH}"
            --cpp_out "${WBT_GENERATED_PROTOBUF_PATH}"
            -I "${WBT_PROTO_PATH}"
            --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
            "${WBT_PROTO_FILE}"
            DEPENDS "${WBT_PROTO_FILE}" USES_TERMINAL)
    list(APPEND WBT_PROTO_HDRS ${WBT_PROTO_HDR})
    list(APPEND WBT_PROTO_HDRS ${WBT_PROTO_GRPC_HDR})
    list(APPEND WBT_PROTO_SRCS ${WBT_PROTO_SRC})
    list(APPEND WBT_PROTO_SRCS ${WBT_PROTO_GRPC_SRC})
endforeach()