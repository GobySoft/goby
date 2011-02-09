file(GLOB PROTOS *.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

file(GLOB SRC *.cpp *.c)
add_library(tes_util SHARED ${SRC} ${PROTO_SRCS} ${PROTO_HDRS})

TARGET_LINK_LIBRARIES(tes_util
  goby_logger ${MOOS_LIBRARIES} ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY})
