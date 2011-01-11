

option(build_util_logger "Build Goby Logger library" ON)
option(build_util_linebasedcomms "Build Line-Based Communications (Ethernet & Serial) library" ON)
option(build_util_tools "Build the Utility tools" OFF)

if(build_util_logger) 
  add_subdirectory(liblogger)
endif()

if(build_util_linebasedcomms) 
  add_subdirectory(liblinebasedcomms)
endif()

if(build_util_tools)
  add_subdirectory(tools)
endif()

