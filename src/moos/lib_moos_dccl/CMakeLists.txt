file(GLOB SRC *.cpp *.c)
add_library(moos_dccl SHARED ${SRC})

target_link_libraries(moos_dccl goby_dccl goby_linebasedcomms)
