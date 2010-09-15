include_directories(${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS server_request.proto)

file(GLOB SRC *.cpp)
add_library(goby_core SHARED ${SRC} ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(goby_core ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} pthread)

