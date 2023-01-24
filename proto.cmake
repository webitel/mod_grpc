
file(MAKE_DIRECTORY ${GENERATED_PROTOBUF_PATH})
set(PROTO_PATH "${CMAKE_SOURCE_DIR}/src/protos")

file(GLOB_RECURSE proto_files ABSOLUTE ${PROTO_PATH}/*.proto)

# Generate Protobuf cpp sources
set(PROTO_HDRS)
set(PROTO_SRCS)
foreach(PROTO_FILE IN LISTS proto_files)
    #message(STATUS "protoc proto(cc): ${PROTO_FILE}")
#    get_filename_component(PROTO_DIR ${PROTO_FILE} DIRECTORY)
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    set(PROTO_HDR ${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.pb.h)
    set(PROTO_SRC ${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.pb.cc)
    set(PROTO_GRPC_HDR ${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.grpc.pb.h)
    set(PROTO_GRPC_SRC ${GENERATED_PROTOBUF_PATH}/${PROTO_NAME}.grpc.pb.cc)
#    message(STATUS "protoc hdr: ${PROTO_HDR}")
#    message(STATUS "protoc src: ${PROTO_SRC}")
    add_custom_command(
            OUTPUT ${PROTO_SRC} ${PROTO_HDR} ${PROTO_GRPC_SRC} ${PROTO_GRPC_HDR}
            COMMAND ${_PROTOBUF_PROTOC}
            ARGS --grpc_out "${GENERATED_PROTOBUF_PATH}"
            --cpp_out "${GENERATED_PROTOBUF_PATH}"
            -I "${PROTO_PATH}"
            --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
            "${PROTO_FILE}"
            DEPENDS "${PROTO_FILE}")
    list(APPEND PROTO_HDRS ${PROTO_HDR})
    list(APPEND PROTO_HDRS ${PROTO_GRPC_HDR})
    list(APPEND PROTO_SRCS ${PROTO_SRC})
    list(APPEND PROTO_SRCS ${PROTO_GRPC_SRC})
endforeach()