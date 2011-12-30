add_executable(test_zero_mq_node2 test.cpp)
target_link_libraries(test_zero_mq_node2 goby_common)

add_test(test_zero_mq_node2 ${goby_BIN_DIR}/test_zero_mq_node2)

