set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIR})
  
  file(GLOB SRC *.cpp *.c)
  add_library(modemdriver SHARED ${SRC})
  target_link_libraries(modemdriver streamlogger serial)
endif()

add_subdirectory(examples)