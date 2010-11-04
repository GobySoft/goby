set(SRC flex_ostreambuf.cpp flex_ostream.cpp logger_manipulators.cpp term_color.cpp)
set(NCURSES_SRC flex_ncurses.cpp)

if(CURSES_FOUND)
  add_library(goby_logger SHARED ${SRC} ${NCURSES_SRC})
  target_link_libraries(goby_logger ncurses ${Boost_LIBRARIES})
else()
  message(WARNING "Missing NCurses so building goby_logger without verbosity=gui support")
  add_library(goby_logger SHARED ${SRC})
  target_link_libraries(goby_logger ${Boost_LIBRARIES})
endif()


