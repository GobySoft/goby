FILE(GLOB SRC *.cpp *.c)

ADD_LIBRARY(nurc_moosapp SHARED ${SRC})
