protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)

add_executable(test_queue1 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_queue1 goby_queue)

add_test(test_queue1 test_queue1)
