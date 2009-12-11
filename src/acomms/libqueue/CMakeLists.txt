#--------------------------------------------------------
# The CMakeLists.txt for:                    lib_queue
# Author(s):                                 t. schneider
#--------------------------------------------------------

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if(Boost_FOUND AND Xerces_LIB_FOUND AND Xerces_H_FOUND )
  include_directories(${Boost_INCLUDE_DIR})
  
  file(GLOB SRC *.cpp *.c)
  add_library(queue SHARED ${SRC})
  target_link_libraries(queue streamlogger xerces-c)
endif()