file(GLOB PROTOS *.proto)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})
add_library(goby_protobuf  
  dynamic_protobuf_manager.cpp 
  ${PROTO_SRCS} ${PROTO_HDRS} ${PROTOBUF_LIBRARY})

target_link_libraries(goby_protobuf ${PROTOBUF_LIBRARY} pthread)
set_target_properties(goby_protobuf PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")

