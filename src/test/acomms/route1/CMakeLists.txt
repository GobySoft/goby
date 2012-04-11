protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)

add_executable(test_route1 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_route1 goby_acomms)

add_test(test_route1 ${goby_BIN_DIR}/test_route1)

