include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(gobyd gobyd.cpp connected_client.cpp)

target_link_libraries(gobyd 
  ${PROTOBUF_LIBRARY}
  ${Boost_LIBRARIES}
  pthread
  goby_core
  goby_logger
  goby_dbo)
