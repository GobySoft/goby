set(build_util ON CACHE BOOL "Build the Goby Utility libraries and applications")
set(build_acomms ON CACHE BOOL "Build the Goby Acoustic Communications librariesand applications")
set(build_core ON CACHE BOOL "Build the Goby Core libraries and applications")
  
set(build_test OFF CACHE BOOL "Build test applications")

if(build_util)
  add_subdirectory(util)
endif()

if(build_acomms)
  add_subdirectory(acomms)
endif()

if(build_core)
  add_subdirectory(core)
endif()
