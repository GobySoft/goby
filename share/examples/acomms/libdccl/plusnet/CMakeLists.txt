add_executable(plusnet plusnet.cpp)
target_link_libraries(plusnet goby_dccl goby_logger goby_protobuf protobuf)
