
# for buggy cdk headers
include_directories(/opt/local/include/cdk)
include_directories(/usr/include/cdk)
include_directories(/sw/include/cdk)


file(GLOB PROTOS *.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})


add_executable(iCommander 
  iCommanderMain.cpp
  iCommander.cpp 
  commander_cdk.cpp
  command_gui.cpp ${PROTO_SRCS} ${PROTO_HDRS})

    
target_link_libraries(iCommander
  m
  pthread
  goby_moos_util
  goby_transitional
  cdk
  ncurses
  ${MOOS_LIBRARIES}
  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARY})
