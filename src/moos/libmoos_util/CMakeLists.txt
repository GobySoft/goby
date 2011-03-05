file(GLOB PROTOS *.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

file(GLOB SRC *.cpp *.c)
add_library(goby_moos_util SHARED ${SRC} ${PROTO_SRCS} ${PROTO_HDRS})

TARGET_LINK_LIBRARIES(goby_moos_util
  goby_logger   goby_core goby_protobuf ${MOOS_LIBRARIES} ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY})
