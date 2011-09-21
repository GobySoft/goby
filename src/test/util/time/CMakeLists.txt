add_executable(test_time time.cpp)
target_link_libraries(test_time goby_util)
add_test(test_time test_time)
