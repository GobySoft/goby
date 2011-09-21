protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ../dccl3/test.proto)

add_executable(test_queue2 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_queue2 goby_acomms)

add_test(test_queue2 ${goby_BIN_DIR}/test_queue2)
