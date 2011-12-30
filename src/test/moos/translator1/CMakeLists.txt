get_filename_component(translator_test_dir ./ ABSOLUTE)
add_definitions(-DTRANSLATOR_TEST_DIR="${translator_test_dir}")

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ../../acomms/dccl1/test.proto basic_node_report.proto)

add_executable(test_translator1 test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(test_translator1 goby_moos)

add_test(test_translator1 ${goby_BIN_DIR}/test_translator1)
