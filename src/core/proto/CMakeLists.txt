protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS server_request.proto)

add_library(goby_core_proto SHARED ${PROTO_SRCS} ${PROTO_HDRS})
