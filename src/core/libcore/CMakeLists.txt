file(GLOB SRC *.cpp)
add_library(goby_core SHARED ${SRC})

target_link_libraries(goby_core ${Boost_LIBRARIES} pthread goby_core_proto goby_logger goby_dbo ${PROTOBUF_LIBRARIES})

