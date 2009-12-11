file(GLOB SRC *.cpp *.c)
add_library(streamlogger SHARED ${SRC})

target_link_libraries(streamlogger ${Boost_LIBRARIES})

