
set(PROTOS abc_frontseat_driver.proto abc_frontseat_driver_config.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SRC abc_frontseat_driver.cpp)

add_library(goby_frontseat_abc SHARED ${SRC} ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(goby_frontseat_abc
  goby_moos
  ${PROTOBUF_LIBRARIES})
