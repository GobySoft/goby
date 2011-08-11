protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ../../../libdccl/test/dccl3/test.proto)

add_executable(test_queue3 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_queue3 goby_queue)

add_test(test_queue3 test_queue3)
