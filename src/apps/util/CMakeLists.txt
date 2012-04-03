add_subdirectory(serial2tcp_server)

if(enable_wt AND enable_zeromq)
  add_subdirectory(goby_store_server)
endif()
