## xerces
goby_find_required_package(Xerces libxerces-c-dev)

option(build_acomms_dccl "Build the Dynamic Compact Control Language library" ON)
option(build_acomms_modemdriver "Build the Acoustic Modem Driver library" ON)
option(build_acomms_queue "Build the Priority Queuing library" ON)
option(build_acomms_amac "Build the Acoustic MAC library" ON)
option(build_acomms_examples "Build the Acoustic Communications examples" ON)
option(build_acomms_tools "Build the Acoustic Communications tools" ON)

if(build_acomms_dccl) 
  add_subdirectory(libdccl)
endif()

if(build_acomms_modemdriver)
  add_subdirectory(libmodemdriver)
endif()

if(build_acomms_queue)
  add_subdirectory(libqueue)
endif()

if(build_acomms_amac)
  add_subdirectory(libamac)
endif()

if(build_acomms_examples)
  add_subdirectory(examples)
endif()
