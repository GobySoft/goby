set(build_dccl ON CACHE BOOL "Build the Dynamic Compact Control Language library")
set(build_modemdriver ON CACHE BOOL "Build the Acoustic Modem Driver library")
set(build_queue ON CACHE BOOL "Build the Priority Queuing library")
set(build_amac ON CACHE BOOL "Build the Acoustic MAC library")
set(build_acomms_examples ON CACHE BOOL "Build the Acoustic Communications examples")
set(build_acomms_tools ON CACHE BOOL "Build the Acoustic Communications tools")

if(build_dccl) 
  add_subdirectory(libdccl)
endif()

if(build_modemdriver)
  add_subdirectory(libmodemdriver)
endif()

if(build_queue)
  add_subdirectory(libqueue)
endif()

if(build_amac)
  add_subdirectory(libamac)
endif()

if(build_acomms_examples)
  add_subdirectory(examples)
endif()