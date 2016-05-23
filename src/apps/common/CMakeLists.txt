
if(enable_wt AND enable_zeromq)
  add_subdirectory(liaison)
endif()

if(enable_hdf5)
  add_subdirectory(goby_hdf5)
endif()
