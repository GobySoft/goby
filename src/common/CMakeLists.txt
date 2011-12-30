file(GLOB PROTOS protobuf/*.proto)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

if(ZEROMQ_FOUND)
  add_library(goby_common 
    application_base.cpp
    zeromq_service.cpp
    ${PROTO_SRCS} ${PROTO_HDRS} ${PROTOBUF_LIBRARY})
  target_link_libraries(goby_common ${PROTOBUF_LIBRARY} pthread zmq)
else()
  add_library(goby_common ${PROTO_SRCS} ${PROTO_HDRS} ${PROTOBUF_LIBRARY})
  target_link_libraries(goby_common ${PROTOBUF_LIBRARY} pthread)
endif()


set_target_properties(goby_common PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")

