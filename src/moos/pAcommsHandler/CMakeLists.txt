file(GLOB PROTOS *.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

add_executable(pAcommsHandler
  pAcommsHandlerMain.cpp
  pAcommsHandler.cpp ${PROTO_SRCS} ${PROTO_HDRS})
  
target_link_libraries(pAcommsHandler
  m
  pthread
  goby_moos_util
  goby_dccl
  goby_queue
  goby_amac
  goby_modemdriver
  goby_logger
  ${MOOS_LIBRARIES}
  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARY})
