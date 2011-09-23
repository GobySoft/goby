
add_executable(test_transitional1 test.cpp)
target_link_libraries(test_transitional1 goby_transitional)

add_test(test_transitional1 ${goby_BIN_DIR}/test_transitional1)
