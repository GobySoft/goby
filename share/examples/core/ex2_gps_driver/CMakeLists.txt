# tells CMake to generate the *.pb.h and *.pb.cc files from the *.proto
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS 
  node_report.proto
  gps_nmea.proto 
  config.proto 
  depth_reading.proto)

# add these executables to the project
add_executable(gps_driver_g gps_driver.cpp ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(node_reporter_g node_reporter.cpp ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(depth_simulator_g depth_simulator.cpp ${PROTO_SRCS} ${PROTO_HDRS})

# and link in the goby_core library
target_link_libraries(gps_driver_g goby_core goby_linebasedcomms)
target_link_libraries(node_reporter_g goby_core)
target_link_libraries(depth_simulator_g goby_core)

