protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS goby_double.proto)

add_executable(proto_test1 proto_test1.cpp ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(proto_test2 proto_test2.cpp ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(proto_test3 proto_test3.cpp ${PROTO_SRCS} ${PROTO_HDRS})

add_executable(interprocess1 interprocess1.cpp)
add_executable(interprocess2 interprocess2.cpp)

target_link_libraries(proto_test1 ${PROTOBUF_LIBRARY} ${Boost_LIBRARIES} pthread wtdbo wtdbosqlite3)
target_link_libraries(proto_test2 ${PROTOBUF_LIBRARY} ${Boost_LIBRARIES} pthread wtdbo wtdbosqlite3 goby_dbo goby_logger)
target_link_libraries(proto_test3 ${PROTOBUF_LIBRARY} ${Boost_LIBRARIES} pthread wtdbo wtdbosqlite3)

target_link_libraries(interprocess1 ${Boost_LIBRARIES} pthread)
target_link_libraries(interprocess2 ${Boost_LIBRARIES} pthread)
