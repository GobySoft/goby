protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)

add_executable(goby_test_dccl9 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(goby_test_dccl9 goby_acomms dccl_ccl_compat)

add_test(goby_test_dccl9 ${goby_BIN_DIR}/goby_test_dccl9)
