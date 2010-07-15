file(GLOB SRC *.cpp *.c)
add_library(goby_asioclient SHARED ${SRC})

target_link_libraries(goby_asioclient ${Boost_LIBRARIES})

