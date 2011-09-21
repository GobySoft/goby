if(CURSES_FOUND)
  protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS chat.proto)
  include_directories(${CMAKE_CURRENT_BINARY_DIR})

  add_executable(goby_chat chat.cpp chat_curses.cpp ${PROTO_SRCS} ${PROTO_HDRS})
  target_link_libraries(goby_chat ${Boost_LIBRARIES} goby_acomms)
else()
  message(WARNING "Missing NCurses so not building `chat` example")
endif()