add_executable(test_udpdriver1 test.cpp)
target_link_libraries(test_udpdriver1 goby_acomms)

add_test(test_udpdriver1 ${goby_BIN_DIR}/test_udpdriver1)

