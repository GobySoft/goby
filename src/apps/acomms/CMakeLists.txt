add_subdirectory(abc_modem_simulator)

if(make_proto_runtime_compile)
  add_subdirectory(analyze_dccl)
endif()

# for now, we need to be able to link in UDPDriver and PBDriver
# at some point, make these dynamically loadable
if(build_pb AND enable_zeromq)
  add_subdirectory(goby_bridge)
  add_subdirectory(goby_file_transfer)
  add_subdirectory(goby_modemdriver)
  add_subdirectory(goby_mosh_relay)
  add_subdirectory(goby_rudics_shore)
  if(enable_sqlite)
    add_subdirectory(goby_store_server)
  endif()
endif()

