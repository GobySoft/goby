# tells CMake to generate the *.pb.h and *.pb.cc files from the *.proto
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS hello_world.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})


# add these executables to the project
add_executable(hello_world1_g hello_world1.cpp ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(hello_world2_g hello_world2.cpp ${PROTO_SRCS} ${PROTO_HDRS})

# and link in the goby_core library
target_link_libraries(hello_world1_g goby_core)
target_link_libraries(hello_world2_g goby_core)

