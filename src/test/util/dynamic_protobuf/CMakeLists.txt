protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test_a.proto)
add_executable(test_dynamic_protobuf dynamic_protobuf.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_dynamic_protobuf dl goby_util goby_protobuf ${Boost_LIBRARIES} ${PROTOBUF_LIBRARIES})
add_test(test_dynamic_protobuf ${goby_BIN_DIR}/test_dynamic_protobuf)


protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test_b.proto)
add_library(test_dyn_protobuf SHARED ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_dyn_protobuf dl ${PROTOBUF_LIBRARIES})
