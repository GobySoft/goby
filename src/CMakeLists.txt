cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)
project(goby)
     
# set built file locations
get_filename_component(goby_LIB_DIR ../lib ABSOLUTE)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${goby_LIB_DIR} CACHE PATH 
  "Output directory for the libraries" )

get_filename_component(goby_BIN_DIR ../bin ABSOLUTE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${goby_BIN_DIR} CACHE PATH
  "Output directory for the binaries" )

#install(DIRECTORY ${goby_BIN_DIR}/ DESTINATION /usr/local/bin FILE_PERMISSIONS WORLD_EXECUTE)
install(DIRECTORY ${goby_LIB_DIR}/ DESTINATION /usr/local/lib)

find_package(Boost 1.34.0 REQUIRED date_time thread regex)  

find_library(Xerces_LIB_FOUND xerces-c)
find_file(Xerces_H_FOUND xercesc/sax2/SAX2XMLReader.hpp)

add_subdirectory(acomms)
