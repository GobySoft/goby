file(GLOB PROTOS protobuf/*.proto)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

add_library(goby_acomms 
  dccl/dccl.cpp
  dccl/dccl_field_codec.cpp
  dccl/dccl_field_codec_helpers.cpp
  dccl/dccl_field_codec_default.cpp
  dccl/dccl_field_codec_default_message.cpp
  dccl/dccl_field_codec_manager.cpp
  dccl/dccl_type_helper.cpp
  dccl/dccl_bitset.cpp
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

target_link_libraries(goby_acomms goby_util goby_common ${Cryptopp_LIBRARIES} ${Boost_LIBRARIES})

set_target_properties(goby_acomms PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")

