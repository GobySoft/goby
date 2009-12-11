file(GLOB SRC *.cpp *.c)
add_library(serial SHARED ${SRC})

target_link_libraries(serial ${Boost_LIBRARIES})

