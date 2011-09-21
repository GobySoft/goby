set(CMAKE_SKIP_RPATH OFF)

if(build_acomms)
  add_subdirectory(acomms)
endif()

if(build_util)
  add_subdirectory(util)
endif()

