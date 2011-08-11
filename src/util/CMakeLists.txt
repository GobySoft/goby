set(SRC
  logger/flex_ostreambuf.cpp 
  logger/flex_ostream.cpp 
  logger/logger_manipulators.cpp 
  logger/term_color.cpp
  linebasedcomms/client_base.h
  linebasedcomms/CMakeLists.txt
  linebasedcomms/connection.h
  linebasedcomms/interface.cpp
  linebasedcomms/interface.h
  linebasedcomms/nmea_sentence.cpp
  linebasedcomms/nmea_sentence.h
  linebasedcomms/serial_client.cpp
  linebasedcomms/serial_client.h
  linebasedcomms/tcp_client.cpp
  linebasedcomms/tcp_client.h
  linebasedcomms/tcp_server.cpp
  linebasedcomms/tcp_server.h
  )

set(NCURSES_SRC
  logger/flex_ncurses.cpp
  )

if(CURSES_FOUND)
  add_library(goby_util SHARED ${SRC} ${NCURSES_SRC})
  target_link_libraries(goby_util ncurses)
else()
  message(WARNING "Missing NCurses so building goby_util without verbosity=gui support")
  add_library(goby_util SHARED ${SRC})
endif()
target_link_libraries(goby_util goby_protobuf ${Boost_LIBRARIES})

set_target_properties(goby_util PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")

option(build_util_tools "Build the Utility tools" OFF)

if(build_util_tools)
  add_subdirectory(tools)
endif()

if(enable_testing)
  add_subdirectory(test)
endif()
