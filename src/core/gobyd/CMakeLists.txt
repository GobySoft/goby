add_executable(gobyd gobyd.cpp connected_client.cpp main.cpp)

target_link_libraries(gobyd 
  ${Boost_LIBRARIES}
  pthread
  goby_core
  goby_logger
  goby_dbo)
