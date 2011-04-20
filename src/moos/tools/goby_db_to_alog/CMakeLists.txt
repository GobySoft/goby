protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS 
  goby_db_to_alog_config.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(goby_db_to_alog goby_db_to_alog.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(goby_db_to_alog goby_core ${PROTOBUF_LIBRARY})

