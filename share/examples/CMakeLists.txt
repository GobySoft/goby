cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)
project(goby-examples)


set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules")
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/../..;${CMAKE_PREFIX_PATH}")
find_package(Goby REQUIRED)

set(CURSES_NEED_NCURSES TRUE)
find_package(Curses)
if(CURSES_FOUND)
  add_definitions(-DHAS_NCURSES)
endif()

find_package(Boost 1.36.0 COMPONENTS system date_time thread program_options filesystem signals)

link_directories(${GOBY_LIBRARY_PATH})
include_directories(${GOBY_INCLUDE_DIRS})

find_package(ProtobufGobyExamples REQUIRED)
protobuf_include_dirs(${GOBY_INCLUDE_DIRS})


add_subdirectory(acomms)
add_subdirectory(util)
add_subdirectory(moos)
