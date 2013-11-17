file(GLOB PROTOS protobuf/*.proto)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

set(SRC
  dccl/dccl.cpp
  queue/queue.cpp
  queue/queue_manager.cpp
  amac/mac_manager.cpp
  modemdriver/abc_driver.cpp
  modemdriver/driver_base.cpp
  modemdriver/mm_driver.cpp
  modemdriver/udp_driver.cpp
  route/route.cpp
  ${PROTO_SRCS} ${PROTO_HDRS}
)

add_library(goby_acomms ${SRC})

target_link_libraries(goby_acomms goby_util goby_common dccl ${Cryptopp_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${GMP_LIBRARIES})

set_target_properties(goby_acomms PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")
