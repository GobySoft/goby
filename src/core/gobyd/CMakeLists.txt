include_directories(${CMAKE_CURRENT_BINARY_DIR})


add_executable(gobyd gobyd.cpp ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(gobyd ${PROTOBUF_LIBRARY} ${Boost_LIBRARIES} pthread)
