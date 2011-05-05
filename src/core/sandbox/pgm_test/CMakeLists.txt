add_executable(pgmtest1 pgmtest1.cpp)
add_executable(pgmtest2 pgmtest2.cpp)
target_link_libraries(pgmtest1 zmq)
target_link_libraries(pgmtest2 zmq)
