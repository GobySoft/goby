file(GLOB PROTOS *.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(pREMUSCodec ${SRC} ${PROTO_SRCS} ${PROTO_HDRS})
      
TARGET_LINK_LIBRARIES(pREMUSCodec
  m
  pthread
  goby_moos_util
  ${MOOS_LIBRARIES}
)
