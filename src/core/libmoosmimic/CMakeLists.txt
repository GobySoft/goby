file(GLOB SRC *.cpp)

add_library(goby_moosmimic SHARED ${SRC} ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(goby_moosmimic ${Boost_LIBRARIES} pthread goby_core goby_protobuf goby_logger ${MOOS_LIBRARIES})
set_target_properties(goby_moosmimic PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")
