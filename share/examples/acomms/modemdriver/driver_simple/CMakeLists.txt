add_executable(driver_simple driver_simple.cpp)
target_link_libraries(driver_simple ${GOBY_LIBRARIES} ${Boost_LIBRARIES} ${PROTOBUF_LIBRARIES})