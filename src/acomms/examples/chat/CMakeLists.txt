add_executable(chat chat.cpp chat_curses.cpp)
target_link_libraries(chat ${Boost_LIBRARIES} queue amac dccl modemdriver)
