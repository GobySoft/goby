add_executable(MOOS_Ex3 main.cpp Simulator.cpp)

target_link_libraries(MOOS_Ex3 ${MOOS_LIBRARIES} goby_moosmimic pthread)
