goby_find_required_package(MOOS "moos")
include_directories(${MOOS_INCLUDE_DIRS})

file(GLOB SRC *.cpp)

add_library(goby_moosmimic SHARED ${SRC} ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(goby_moosmimic ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} pthread goby_core goby_core_proto goby_logger ${MOOS_LIBRARIES})
