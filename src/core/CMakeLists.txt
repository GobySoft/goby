
if(build_test)
  add_subdirectory(test)
endif()

add_subdirectory(gobyd)

option(build_core_dbo "Build the Database Objects Core Library" ON)
option(build_core_core "Build the Core Library" ON)
option(build_core_proto "Build the Core Protobuf library" ON)
option(build_core_moosmimic "Build the Core MOOS Mimic Library (requires MOOS)" OFF)
option(build_core_examples "Build the Core Examples" OFF)

if(build_core_dbo)
  add_subdirectory(libdbo)
endif()

if(build_core_core)
  add_subdirectory(libcore)
endif()

if(build_core_moosmimic)
  add_subdirectory(libmoosmimic)
endif()

if(build_core_proto)
  add_subdirectory(proto)
endif()

if(build_core_examples)
  add_subdirectory(examples)
endif()