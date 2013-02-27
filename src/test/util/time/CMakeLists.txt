add_executable(goby_test_time time.cpp)
target_link_libraries(goby_test_time goby_util)
add_test(goby_test_time ${goby_BIN_DIR}/goby_test_time)
