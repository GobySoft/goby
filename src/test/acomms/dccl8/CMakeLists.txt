protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto ../dccl3/header.proto)

add_executable(test_dccl8 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_dccl8 goby_acomms)

add_test(test_dccl8 ${goby_BIN_DIR}/test_dccl8)
