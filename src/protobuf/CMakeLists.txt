file(GLOB PROTOS *.proto)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})
add_library(goby_protobuf SHARED ${PROTO_SRCS} ${PROTO_HDRS} ${PROTOBUF_LIBRARY})

target_link_libraries(goby_protobuf ${PROTOBUF_LIBRARY})
