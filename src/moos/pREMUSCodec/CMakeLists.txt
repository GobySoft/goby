FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(pREMUSCodec ${SRC})
      
TARGET_LINK_LIBRARIES(pREMUSCodec
  m
  pthread
  goby_moos_util
  ${MOOS_LIBRARIES}
)
