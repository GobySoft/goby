#--------------------------------------------------------
# The CMakeLists.txt for:                    lib_mmdriver
# Author(s):                                 t. schneider
#--------------------------------------------------------

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIR})
  
  file(GLOB SRC *.cpp *.c)
  add_library(mmdriver SHARED ${SRC})
  target_link_libraries(mmdriver flexcout)
endif()