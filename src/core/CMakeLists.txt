

if(GCC_COMPILER_VERSION MATCHES ".*4\\.4\\.4\\-14ubuntu5.*")
  message(" ")
  message("    !!!!! WARNING Your compiler is BUGGY. !!!!! ")
  message(" ")
  message("    If possible, upgrade your compiler to e.g. g++ 4.5:")
  message(" ")
  message("       $ sudo apt-get install g++-4.5")
  message(" ")
  message("    And build using that compiler cmake -DCMAKE_CXX_COMPILER=g++-4.5")
  message(" ")
  message("    we will now disable all assertions as a work around, by")
  message("    building using -DNDEBUG. You will need to define this")
  message("    also for programs built using Goby")
  message(" ")
  add_definitions(-DNDEBUG)
endif(GCC_COMPILER_VERSION MATCHES ".*4\\.4\\.4\\-14ubuntu5.*")


if(BUILD_TESTING)
  add_subdirectory(test)
endif()


option(build_core_lib "Build the Core Libraries" ON)
option(build_core_bin "Build the Core Binaries" ON)

if(build_core_lib)
  add_subdirectory(libdbo)
  add_subdirectory(libcore)
  
  if(MOOS_FOUND)
    option(build_core_moosmimic "Build the Core MOOS Mimic Library (requires MOOS)" ON)
  else()
    option(build_core_moosmimic "Build the Core MOOS Mimic Library (requires MOOS)" OFF)
  endif()

  if(build_core_moosmimic)
#    add_subdirectory(libmoosmimic)
  endif()
endif()

if(build_core_bin)
 # add_subdirectory(gobyd)
  add_subdirectory(database)
  add_subdirectory(liaison)
endif()
