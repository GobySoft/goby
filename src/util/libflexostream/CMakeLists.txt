file(GLOB SRC *.cpp *.c)
add_library(flexostream SHARED ${SRC})

target_link_libraries(flexostream ncurses ${Boost_LIBRARIES})

