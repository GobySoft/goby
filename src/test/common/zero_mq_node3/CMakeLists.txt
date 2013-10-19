add_executable(goby_test_zero_mq_node3 test.cpp)
target_link_libraries(goby_test_zero_mq_node3  goby_common)

if(enable_testing_zmq)
    add_test(goby_test_zero_mq_node3 ${goby_BIN_DIR}/goby_test_zero_mq_node3)
endif()
