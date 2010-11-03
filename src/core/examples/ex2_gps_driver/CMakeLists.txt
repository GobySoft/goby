# tells CMake to generate the *.pb.h and *.pb.cc files from the *.proto
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS node_report.proto gps_nmea.proto config.proto)

# add these executables to the project
add_executable(gps_driver_g gps_driver.cpp ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(example_ctd_simulator_g ctd_simulator.cpp ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(example_sonar_simulator_g ctd_simulator.cpp ${PROTO_SRCS} ${PROTO_HDRS})

# and link in the goby_core library
target_link_libraries(gps_driver_g goby_core goby_linebasedcomms)
target_link_libraries(example_ctd_simulator_g goby_core)
target_link_libraries(example_sonar_simulator_g goby_core)

