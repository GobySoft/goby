
add_executable(goby_test_transitional1 test.cpp)
target_link_libraries(goby_test_transitional1 goby_moos)

add_test(goby_test_transitional1 ${goby_BIN_DIR}/goby_test_transitional1)
