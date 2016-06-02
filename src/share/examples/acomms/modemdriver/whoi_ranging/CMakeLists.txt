add_executable(whoi_ranging whoi_ranging.cpp)
target_link_libraries(whoi_ranging goby_acomms ${Boost_LIBRARIES} ${PROTOBUF_LIBRARIES})
install(TARGETS whoi_ranging RUNTIME DESTINATION bin)
