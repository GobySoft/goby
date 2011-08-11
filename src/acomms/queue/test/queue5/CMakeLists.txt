protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)

add_executable(test_queue5 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_queue5 goby_queue)

add_test(test_queue5 test_queue5)
