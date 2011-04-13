protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)

add_executable(test_dccl1 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_dccl1 goby_dccl)

add_test(test_dccl1 test_dccl1)
