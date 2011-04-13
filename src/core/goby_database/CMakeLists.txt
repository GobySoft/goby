add_executable(goby_database goby_database.cpp hello_world.pb.cc)

target_link_libraries(goby_database
  ${Boost_LIBRARIES}
  pthread
  goby_core
  goby_logger
  goby_dbo)
