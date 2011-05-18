cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)
project(goby-examples)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(Goby REQUIRED)

set(CURSES_NEED_NCURSES TRUE)
find_package(Curses)
if(CURSES_FOUND)
  add_definitions(-DHAS_NCURSES)
endif()

find_package(Boost 1.36.0 COMPONENTS system date_time thread regex program_options signals filesystem)
include_directories(${Boost_INCLUDE_DIRS})

find_package(Cryptopp REQUIRED)

link_directories(${GOBY_LIBRARY_PATH})
include_directories(${GOBY_INCLUDE_DIRS})

option(workaround_2_4_kernel "Apply workarounds for Linux 2.4 kernel" OFF)
if(workaround_2_4_kernel)
	add_definitions(-DBOOST_ASIO_DISABLE_EPOLL)
endif()

add_subdirectory(acomms)
add_subdirectory(util)
