#--------------------------------------------------------
# The CMakeLists.txt for:                    lib_tes_util
# Author(s):                                 t. schneider
#--------------------------------------------------------

FILE(GLOB SRC *.cpp *.c)
ADD_LIBRARY(flexcout SHARED ${SRC})

TARGET_LINK_LIBRARIES(flexcout ncurses ${Boost_LIBRARIES})
