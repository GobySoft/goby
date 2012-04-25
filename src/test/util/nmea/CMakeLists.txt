add_executable(test_nmea nmea.cpp)
target_link_libraries(test_nmea goby_util)
add_test(test_nmea ${goby_BIN_DIR}/test_nmea)
