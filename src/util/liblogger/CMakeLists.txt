file(GLOB SRC *.cpp *.c)
add_library(goby_logger SHARED ${SRC})

target_link_libraries(goby_logger ncurses ${Boost_LIBRARIES})

