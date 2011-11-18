file(GLOB PROTOS *.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

add_executable(pAcommsHandler
  pAcommsHandlerMain.cpp
  pAcommsHandler.cpp ${PROTO_SRCS} ${PROTO_HDRS})
  
target_link_libraries(pAcommsHandler
  m
  pthread
  goby_acomms
  goby_util
  goby_moos
  ${MOOS_LIBRARIES}
  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARY})
