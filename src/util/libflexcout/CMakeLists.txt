file(GLOB SRC *.cpp *.c)
add_library(flexcout SHARED ${SRC})

target_link_libraries(flexcout ncurses streamlogger ${Boost_LIBRARIES})

