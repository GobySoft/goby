file(GLOB SRC *.cpp *.c)
add_library(goby_linebasedcomms SHARED ${SRC})

target_link_libraries(goby_linebasedcomms ${Boost_LIBRARIES})

