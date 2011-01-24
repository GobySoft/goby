

add_executable(iCommander 
  iCommanderMain.cpp
  iCommander.cpp 
  commander_cdk.cpp
  command_gui.cpp)

    
target_link_libraries(iCommander
  m
  pthread
  tes_util
  goby_dccl
  cdk
  ncurses
  ${MOOS_LIBRARIES}
  ${Boost_LIBRARIES})
