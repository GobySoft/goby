add_executable(pAcommsHandler
  pAcommsHandlerMain.cpp
  pAcommsHandler.cpp)
  
target_link_libraries(pAcommsHandler
  m
  tes_util
  pthread
  goby_dccl
  moos_dccl
  goby_queue
  goby_amac
  goby_modemdriver
  goby_logger
  ${MOOS_LIBRARIES}
  ${Boost_LIBRARIES})
