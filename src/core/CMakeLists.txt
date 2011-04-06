
if(BUILD_TESTING)
  add_subdirectory(test)
endif()


option(build_core_lib "Build the Core Libraries" ON)
option(build_core_bin "Build the Core Binaries" ON)

if(build_core_lib)
  add_subdirectory(libdbo)
  add_subdirectory(libcore)

  option(build_core_moosmimic "Build the Core MOOS Mimic Library (requires MOOS)" OFF)
  if(build_core_moosmimic)
    add_subdirectory(libmoosmimic)
  endif()
endif()

if(build_core_bin)
  add_subdirectory(gobyd)
endif()
