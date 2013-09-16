protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
  goby_rudics_shore_config.proto)


add_executable(goby_rudics_shore
  goby_rudics_shore.cpp
  ${PROTO_HDRS}
  ${PROTO_SRCS}
  )

target_link_libraries(
  goby_rudics_shore 
  ${Boost_LIBRARIES}
  goby_common
  goby_util
  goby_pb
  goby_acomms
  )
