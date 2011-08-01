file(GLOB PROTOS *.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

add_executable(perls_driver perls_driver.cpp ${PROTO_SRCS} ${PROTO_HDRS})    
target_link_libraries(perls_driver goby_modemdriver)