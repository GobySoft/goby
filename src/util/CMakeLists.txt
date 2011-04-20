option(build_util_lib "Build Goby Utility libraries" ON)
option(build_util_tools "Build the Utility tools" OFF)

if(build_util_lib) 
  add_subdirectory(liblogger)
  add_subdirectory(liblinebasedcomms)
endif()

if(build_util_tools)
  add_subdirectory(tools)
endif()

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
