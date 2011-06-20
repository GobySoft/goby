file(GLOB PROTOS *.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

add_executable(goby_liaison liaison.cpp liaison_scope.cpp liaison_home.cpp ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(goby_liaison
  ${Boost_LIBRARIES}
  pthread
  goby_core
  goby_logger
  wt
  wthttp
  goby_moos_util
  )

