add_executable(chat chat.cpp chat_curses.cpp)
target_link_libraries(chat ${Boost_LIBRARIES} goby_queue goby_amac goby_dccl goby_modemdriver)
