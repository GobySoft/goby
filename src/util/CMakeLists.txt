set(SRC
  time.cpp
  logger/flex_ostreambuf.cpp 
  logger/flex_ostream.cpp 
  logger/logger_manipulators.cpp 
  logger/term_color.cpp
  linebasedcomms/interface.cpp
  linebasedcomms/nmea_sentence.cpp
  linebasedcomms/serial_client.cpp
  linebasedcomms/tcp_client.cpp
  linebasedcomms/tcp_server.cpp
  dynamic_protobuf_manager.cpp
  )

set(NCURSES_SRC
  logger/flex_ncurses.cpp
  )

if(CURSES_FOUND)
  add_library(goby_util  ${SRC} ${NCURSES_SRC})
  target_link_libraries(goby_util ncurses)
else()
  message(WARNING "Missing NCurses so building goby_util without verbosity=gui support")
  add_library(goby_util  ${SRC})
endif()
target_link_libraries(goby_util goby_protobuf ${Boost_LIBRARIES})

set_target_properties(goby_util PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")
