file(GLOB SRC *.cpp)
add_library(goby_core SHARED ${SRC} ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(goby_core ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} pthread goby_core_proto goby_logger)

