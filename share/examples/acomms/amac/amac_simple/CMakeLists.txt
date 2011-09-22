add_executable(amac_simple amac_simple.cpp)
target_link_libraries(amac_simple ${GOBY_LIBRARIES} ${Boost_LIBRARIES} ${PROTOBUF_LIBRARIES})