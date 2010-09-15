include_directories(${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)

add_executable(test_g test_g.cpp ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(test_g goby_core)
