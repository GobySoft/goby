file(GLOB PROTOS *.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})
add_library(goby_acomms_protobuf SHARED ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(goby_acomms_protobuf goby_core_proto ${PROTOBUF_LIBRARY})
