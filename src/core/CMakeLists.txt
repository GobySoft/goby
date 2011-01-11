## find external packages, libraries, and headers for goby-core

goby_find_required_package(ProtobufGoby "libprotobuf-c0-dev" REQUIRED)

if(build_test)
  add_subdirectory(test)
endif()

add_subdirectory(gobyd)
add_subdirectory(libdbo)
add_subdirectory(libcore)
add_subdirectory(proto)
add_subdirectory(libmoosmimic)
add_subdirectory(examples)
