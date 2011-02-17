## xerces
goby_find_required_package(Xerces libxerces-c-dev)

get_filename_component(dccl_include_dir ${goby_INC_DIR}/goby/acomms/libdccl ABSOLUTE)
add_definitions(-DDCCL_INCLUDE_DIR="${dccl_include_dir}")

option(build_acomms_lib "Build the Acoustic Communications libraries" ON)
option(build_acomms_examples "Build the Acoustic Communications examples" OFF)
option(build_acomms_tools "Build the Acoustic Communications tools" OFF)

if(build_acomms_lib) 
  add_subdirectory(libdccl)
  add_subdirectory(libmodemdriver)
  add_subdirectory(libqueue)
  add_subdirectory(libamac)
  add_subdirectory(protobuf)
endif()

if(build_acomms_examples)
  add_subdirectory(examples)
endif()
