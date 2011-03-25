# tells CMake to generate the *.pb.h and *.pb.cc files from the *.proto
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS hello_acomms.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# add these executables to the project
add_executable(hello_acomms1_g hello_acomms1.cpp ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(hello_acomms2_g hello_acomms2.cpp ${PROTO_SRCS} ${PROTO_HDRS})

# and link in the goby_core library
target_link_libraries(hello_acomms1_g goby_core)
target_link_libraries(hello_acomms2_g goby_core)

