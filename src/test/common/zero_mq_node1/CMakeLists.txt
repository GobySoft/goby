add_executable(goby_test_zero_mq_node1 test.cpp)
target_link_libraries(goby_test_zero_mq_node1 goby_common)

add_test(goby_test_zero_mq_node1 ${goby_BIN_DIR}/goby_test_zero_mq_node1)

