add_executable(iFrontSeat
  iFrontSeat.cpp
  legacy_translator.cpp)

target_link_libraries(iFrontSeat 
  goby_moos
  goby_frontseat_bluefin
  ${MOOS_LIBRARIES}
  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CMAKE_DL_LIBS}
)

