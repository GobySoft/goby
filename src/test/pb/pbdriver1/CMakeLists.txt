add_executable(goby_test_pbdriver1 test.cpp ../../acomms/driver_tester/driver_tester.cpp)
target_link_libraries(goby_test_pbdriver1 goby_acomms goby_pb)

