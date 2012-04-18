add_subdirectory(abc_modem_simulator)
add_subdirectory(analyze_dccl)

# for now, we need to be able to link in UDPDriver and PBDriver
# at some point, make these dynamically loadable
if(build_pb AND enable_zeromq)
  add_subdirectory(goby_bridge)
endif()

if(enable_sqlite AND enable_zeromq)
  add_subdirectory(goby_store_server)
endif()
