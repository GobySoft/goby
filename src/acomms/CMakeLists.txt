## xerces
goby_find_required_package(Xerces libxerces-c-dev)
## crytopp
goby_find_required_package(Cryptopp "libcryptopp-dev")

option(build_acomms_lib "Build the Acoustic Communications libraries" ON)
option(build_acomms_tools "Build the Acoustic Communications tools" ON)

if(build_acomms_lib) 
  add_subdirectory(libdccl)
  add_subdirectory(libmodemdriver)
  add_subdirectory(libqueue)
  add_subdirectory(libamac)
endif()
