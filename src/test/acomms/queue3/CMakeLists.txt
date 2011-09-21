protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ../dccl3/test.proto)

add_executable(test_queue3 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_queue3 goby_acomms)

add_test(test_queue3 ${goby_BIN_DIR}/test_queue3)
