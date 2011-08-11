protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)

add_executable(test_dccl9 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_dccl9 goby_acomms)

add_test(test_dccl9 test_dccl9)
