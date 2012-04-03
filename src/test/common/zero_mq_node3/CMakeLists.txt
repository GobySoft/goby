add_executable(test_zero_mq_node3 test.cpp)
target_link_libraries(test_zero_mq_node3  goby_common)

add_test(test_zero_mq_node3 ${goby_BIN_DIR}/test_zero_mq_node3)

