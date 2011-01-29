## xerces
goby_find_required_package(Xerces libxerces-c-dev)

option(build_acomms_lib "Build the Acoustic Communications libraries" ON)
option(build_acomms_examples "Build the Acoustic Communications examples" OFF)
option(build_acomms_tools "Build the Acoustic Communications tools" OFF)

if(build_acomms_lib) 
  add_subdirectory(libdccl)
  add_subdirectory(libmodemdriver)
  add_subdirectory(libqueue)
  add_subdirectory(libamac)
  add_subdirectory(proto)
endif()

if(build_acomms_examples)
  add_subdirectory(examples)
endif()
