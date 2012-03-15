add_executable(flexostream_simple flexostream_simple.cpp)
target_link_libraries(flexostream_simple ${GOBY_LIBRARIES} ${Boost_LIBRARIES} ${PROTOBUF_LIBRARIES})
install(TARGETS flexostream_simple RUNTIME DESTINATION bin)
