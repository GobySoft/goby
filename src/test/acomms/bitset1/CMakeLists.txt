add_executable(test_bitset1 test.cpp)
target_link_libraries(test_bitset1 goby_acomms)

add_test(test_bitset1 ${goby_BIN_DIR}/test_bitset1)

