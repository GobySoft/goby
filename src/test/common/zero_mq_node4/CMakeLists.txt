add_executable(test_zero_mq_node4 test.cpp)
target_link_libraries(test_zero_mq_node4 goby_pb)

add_test(test_zero_mq_node4 ${goby_BIN_DIR}/test_zero_mq_node4)

