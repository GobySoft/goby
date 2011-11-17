protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto header.proto)

add_executable(test_dccl3 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_dccl3 goby_acomms)

add_test(test_dccl3 ${goby_BIN_DIR}/test_dccl3)
