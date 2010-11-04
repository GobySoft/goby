## find external packages, libraries, and headers for goby-core

find_package(ProtobufGoby REQUIRED)

foreach(I PROTOBUF)
  if(NOT ${I}_FOUND)
    message(FATAL_ERROR "missing required external library: ${I}. read and/or execute the goby/DEPENDENCIES file to resolve.")
  endif()
endforeach()

if(build_test)
  add_subdirectory(test)
endif()

add_subdirectory(gobyd)
add_subdirectory(libdbo)
add_subdirectory(libcore)
add_subdirectory(proto)
add_subdirectory(libmoosmimic)
add_subdirectory(examples)
