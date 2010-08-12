#--------------------------------------------------------
# The CMakeLists.txt for:                    lib_queue
# Author(s):                                 t. schneider
#--------------------------------------------------------

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if(Boost_FOUND AND Xerces_LIB_FOUND AND Xerces_H_FOUND )
  include_directories(${Boost_INCLUDE_DIR})
  
  file(GLOB SRC *.cpp *.c)
  add_library(goby_queue SHARED ${SRC})
  target_link_libraries(goby_queue goby_logger xerces-c goby_dccl)
endif()

if(build_acomms_examples)
  add_subdirectory(examples)
endif()
