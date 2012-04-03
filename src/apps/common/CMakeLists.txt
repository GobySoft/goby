
if(enable_wt AND enable_zeromq)
  add_subdirectory(liaison)
  add_subdirectory(database)
endif()
