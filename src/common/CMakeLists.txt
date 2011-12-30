file(GLOB PROTOS protobuf/*.proto)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

set(SRC
  logger/flex_ostreambuf.cpp 
  logger/flex_ostream.cpp 
  logger/logger_manipulators.cpp 
  logger/term_color.cpp
  configuration_reader.cpp
  ${PROTO_SRCS} ${PROTO_HDRS} 
)

if(ZEROMQ_FOUND)
  set(SRC
    ${SRC}
    application_base.cpp
    zeromq_service.cpp
    )
endif()

if(CURSES_FOUND)
  set(SRC
    ${SRC}
    logger/flex_ncurses.cpp
    )
endif()

add_library(goby_common ${SRC})

if(ZEROMQ_FOUND)
  target_link_libraries(goby_common zmq)
endif()

if(CURSES_FOUND)
  target_link_libraries(goby_common ncurses)
else()
  message(WARNING "Missing NCurses so building goby_common Logger without GUI support")
endif()


target_link_libraries(goby_common ${PROTOBUF_LIBRARY} ${Boost_LIBRARIES} pthread)
set_target_properties(goby_common PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")

