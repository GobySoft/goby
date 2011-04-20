add_executable(test_log test.cpp)
target_link_libraries(test_log goby_logger)
add_test(test_log test_log)
