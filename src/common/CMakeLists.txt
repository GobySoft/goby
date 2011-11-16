file(GLOB PROTOS *.proto)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})
add_library(goby_common ${PROTO_SRCS} ${PROTO_HDRS} ${PROTOBUF_LIBRARY})

target_link_libraries(goby_common ${PROTOBUF_LIBRARY} pthread)
set_target_properties(goby_common PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")

