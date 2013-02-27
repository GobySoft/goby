add_executable(goby_test_udpdriver1 test.cpp)
target_link_libraries(goby_test_udpdriver1 goby_acomms)

add_test(goby_test_udpdriver1 ${goby_BIN_DIR}/goby_test_udpdriver1)

