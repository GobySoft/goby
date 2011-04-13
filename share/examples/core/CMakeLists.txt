find_package(ProtobufGobyExamples REQUIRED)
protobuf_include_dirs(${GOBY_INCLUDE_DIRS})


add_subdirectory(ex1_hello_world)
add_subdirectory(ex2_gps_driver)
add_subdirectory(ex3_hello_acomms)