file(GLOB PROTOS *.proto)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})
add_library(goby_acomms_proto SHARED ${PROTO_SRCS} ${PROTO_HDRS})

