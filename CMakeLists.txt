cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)
project(goby)
     
# set built file locations
get_filename_component(goby_LIB_DIR ./lib ABSOLUTE)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${goby_LIB_DIR} CACHE PATH 
  "Output directory for the libraries" )

get_filename_component(goby_BIN_DIR ./bin ABSOLUTE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${goby_BIN_DIR} CACHE PATH
  "Output directory for the binaries" )

get_filename_component(goby_INC_DIR ./include ABSOLUTE)
get_filename_component(goby_SRC_DIR ./src ABSOLUTE)


# for > make install
install(DIRECTORY ${goby_BIN_DIR}/ DESTINATION /usr/local/bin FILE_PERMISSIONS WORLD_EXECUTE)
install(DIRECTORY ${goby_LIB_DIR}/ DESTINATION /usr/local/lib/goby)
install(DIRECTORY ${goby_INC_DIR}/ DESTINATION /usr/local/include/goby)

# find packages, libraries, and headers
find_package(Boost 1.34.0 REQUIRED date_time thread regex)  

find_package(Curses REQUIRED)

find_library(Xerces_LIB_FOUND xerces-c)
find_file(Xerces_H_FOUND xercesc/sax2/SAX2XMLReader.hpp)

find_file(Asio_H_FOUND asio.hpp)

# include local headers
file(GLOB_RECURSE INCLUDE_FILES RELATIVE ${goby_SRC_DIR} src/*.h)

#message("${INCLUDE_FILES}")

foreach(I ${INCLUDE_FILES})
  configure_file(${goby_SRC_DIR}/${I} ${goby_INC_DIR}/${I})
endforeach()

include_directories(${goby_INC_DIR})


# start adding subdirectories
add_subdirectory(src)
