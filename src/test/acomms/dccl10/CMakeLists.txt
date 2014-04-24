protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)

add_executable(goby_test_dccl10 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(goby_test_dccl10 goby_acomms dccl_arithmetic)

add_test(goby_test_dccl10 ${goby_BIN_DIR}/goby_test_dccl10)
