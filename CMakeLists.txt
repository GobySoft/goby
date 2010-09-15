# t. schneider tes@mit.edu 
# base CMakeLists.txt file for goby
# for help on CMake, see http://www.cmake.org/cmake/help/documentation.html
# or type > man cmake

# this project is intended to be built out of source by using 
# > ./INSTALL

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)
project(goby)
     
# fetch all the local directories for generated code
get_filename_component(goby_LIB_DIR lib ABSOLUTE)
get_filename_component(goby_BIN_DIR bin ABSOLUTE)
get_filename_component(goby_INC_DIR include ABSOLUTE)
get_filename_component(goby_SRC_DIR src ABSOLUTE)

# set the cmake defaults for libraries and binaries
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${goby_LIB_DIR} CACHE PATH 
  "Output directory for the libraries" )
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${goby_BIN_DIR} CACHE PATH
  "Output directory for the binaries" )


# set instructions for `make install`
install(DIRECTORY ${goby_BIN_DIR}/ DESTINATION /usr/local/bin FILE_PERMISSIONS WORLD_EXECUTE)
install(DIRECTORY ${goby_LIB_DIR}/ DESTINATION /usr/local/lib)
install(DIRECTORY ${goby_INC_DIR}/ DESTINATION /usr/local/include/goby)

# allows use to write custom modules or modifying existing ones
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

# find packages, libraries, and headers
find_package(Boost 1.34.0 COMPONENTS date_time thread regex)
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses)
# need custom FindProtoBuf because packaged one is missing options
find_package(ProtobufGoby REQUIRED)
find_library(Xerces_LIB_FOUND xerces-c)
find_file(Xerces_H_FOUND xercesc/sax2/SAX2XMLReader.hpp)
if(Xerces_LIB_FOUND AND Xerces_H_FOUND)
  set(Xerces_FOUND TRUE)
endif()
find_file(Asio_H_FOUND asio.hpp)
if(Asio_H_FOUND)
  set(Asio_FOUND TRUE)
endif()

# give the user a useful warning if third party dependencies
# are missing
foreach(I Boost CURSES Xerces Asio)
  if(NOT ${I}_FOUND)
    message(FATAL_ERROR "missing required external library: ${I}. read and/or execute the goby/DEPENDENCIES file to resolve.")
  endif()
endforeach()


# copy to goby/include
file(GLOB_RECURSE INCLUDE_FILES RELATIVE ${goby_SRC_DIR} src/*.h)
foreach(I ${INCLUDE_FILES})
  configure_file(${goby_SRC_DIR}/${I} ${goby_INC_DIR}/goby/${I})
endforeach()


# let cmake know where the headers are
include_directories(${goby_INC_DIR})
include_directories(${Boost_INCLUDE_DIR})


# start adding subdirectories
add_subdirectory(src)

