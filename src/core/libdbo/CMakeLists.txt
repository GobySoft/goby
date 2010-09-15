file(GLOB SRC *.cpp)
add_library(goby_dbo SHARED ${SRC})
target_link_libraries(goby_dbo ${Boost_LIBRARIES})
