FILE(GLOB SRC *.cpp *.c)

ADD_LIBRARY(tes_util SHARED ${SRC})

TARGET_LINK_LIBRARIES(tes_util nurc_moosapp goby_logger ${MOOS_LIBRARIES} ${Boost_LIBRARIES})
