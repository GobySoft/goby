include_directories("${CMAKE_CURRENT_BINARY_DIR}/proto")

if(build_test)
  add_subdirectory(test)
endif()

add_subdirectory(gobyd)
add_subdirectory(libdbo)
add_subdirectory(libcore)
add_subdirectory(proto)
