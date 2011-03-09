cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)
project(goby-examples)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(Goby REQUIRED)
find_package(ProtobufGobyExamples REQUIRED)

set(CURSES_NEED_NCURSES TRUE)
find_package(Curses)
if(CURSES_FOUND)
  add_definitions(-DHAS_NCURSES)
endif()

link_directories(${GOBY_LIBRARY_PATH})
include_directories(${GOBY_INCLUDE_DIRS})

add_subdirectory(acomms)
add_subdirectory(core)
add_subdirectory(util)
