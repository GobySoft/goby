option(build_core_lib "Build the Core Libraries" ON)

if(build_core_lib)
  add_subdirectory(libcore)
endif()
