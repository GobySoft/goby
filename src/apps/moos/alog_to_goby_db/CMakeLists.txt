protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS 
  alog_to_goby_db_config.proto moos_types.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(alog_to_goby_db alog_to_goby_db.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(alog_to_goby_db goby_dbo goby_moos ${PROTOBUF_LIBRARY})

