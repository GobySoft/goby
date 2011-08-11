add_executable(test_amac1 test.cpp)
target_link_libraries(test_amac1 goby_amac)

add_test(test_amac1 test_amac1)
