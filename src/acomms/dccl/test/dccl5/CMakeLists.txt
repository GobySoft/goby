protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)

add_executable(test_dccl5 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_dccl5 goby_dccl)

add_test(test_dccl5 test_dccl5)
