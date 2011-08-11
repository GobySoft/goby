protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ../dccl3/test.proto)

add_executable(test_queue4 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_queue4 goby_acomms)

add_test(test_queue4 test_queue4)
