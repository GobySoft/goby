file(GLOB SRC *.cpp *.c)
add_library(goby_linebasedcomms SHARED ${SRC})

target_link_libraries(goby_linebasedcomms goby_protobuf ${Boost_LIBRARIES} pthread)
set_target_properties(goby_linebasedcomms PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")

