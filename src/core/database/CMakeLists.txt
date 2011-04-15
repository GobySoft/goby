file(GLOB PROTOS *.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})


add_executable(goby_database goby_database.cpp ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(goby_database
  ${Boost_LIBRARIES}
  pthread
  goby_core
  goby_logger
  goby_dbo)
