set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIR})
  
  file(GLOB SRC *.cpp *.c)
  add_library(goby_modemdriver SHARED ${SRC})
  target_link_libraries(goby_modemdriver goby_logger goby_linebasedcomms)
endif()

add_subdirectory(examples)