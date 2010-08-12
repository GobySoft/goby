set(build_logger ON CACHE BOOL "Build Goby Logger library")
set(build_linebasedcomms ON CACHE BOOL "Build Line-Based Communications (Ethernet & Serial) library")
set(build_util_tools ON CACHE BOOL "Build the Utility tools")

if(build_logger) 
  add_subdirectory(liblogger)
endif()

if(build_linebasedcomms) 
  add_subdirectory(liblinebasedcomms)
endif()

if(build_util_tools)
  add_subdirectory(tools)
endif()
