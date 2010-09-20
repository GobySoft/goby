include_directories(${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)

add_executable(test_g1 test_g1.cpp ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(test_g2 test_g2.cpp ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(test_g1 goby_core)
target_link_libraries(test_g2 goby_core)
