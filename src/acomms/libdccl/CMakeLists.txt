set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if(Boost_FOUND AND Xerces_LIB_FOUND AND Xerces_H_FOUND )
  include_directories(${Boost_INCLUDE_DIR})
  
  file(GLOB SRC *.cpp *.c)
  add_library(goby_dccl SHARED ${SRC})
  # icuuc needed by xerces
  target_link_libraries(goby_dccl goby_logger xerces-c icuuc crypto++ ${Boost_LIBRARIES})
endif()

if(build_acomms_examples)
  add_subdirectory(examples)
endif()

if(build_acomms_tools)
  add_subdirectory(tools)
endif()

