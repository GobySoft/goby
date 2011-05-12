# t. schneider tes@mit.edu 
# base CMakeLists.txt file for goby
# for help on CMake, see http://www.cmake.org/cmake/help/documentation.html
# or type > man cmake

# this project is intended to be built out of source by using 
# > ./INSTALL

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)
project(goby)

## allows us to write custom modules or modifying existing ones
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

enable_testing()
include(CTest)

set(GOBY_VERSION_MAJOR "2")
set(GOBY_VERSION_MINOR "0")
set(GOBY_VERSION_PATCH "0")
set(GOBY_VERSION_PATCH "0~PRE-RELEASE")
set(GOBY_VERSION "${GOBY_VERSION_MAJOR}.${GOBY_VERSION_MINOR}.${GOBY_VERSION_PATCH}")
set(GOBY_VERSION_DATE "2011 Mar 16")
set(GOBY_SOVERSION "2")
     
## fetch all the local directories for generated code
get_filename_component(goby_LIB_DIR lib ABSOLUTE)
get_filename_component(goby_BIN_DIR bin ABSOLUTE)
get_filename_component(goby_INC_DIR include ABSOLUTE)
get_filename_component(goby_SRC_DIR src ABSOLUTE)
get_filename_component(goby_SHARE_DIR share ABSOLUTE)
get_filename_component(goby_BUILD_DIR build ABSOLUTE)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -fprofile-arcs -ftest-coverage")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -fprofile-arcs -ftest-coverage")
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")

## set the cmake defaults for libraries and binaries
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${goby_LIB_DIR} CACHE PATH 
  "Output directory for the libraries" )
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${goby_BIN_DIR} CACHE PATH
  "Output directory for the binaries" )

## set instructions for `make install`
install(DIRECTORY ${goby_BIN_DIR}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/bin 
  FILE_PERMISSIONS WORLD_EXECUTE)
install(DIRECTORY ${goby_LIB_DIR}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(DIRECTORY ${goby_SHARE_DIR}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/share/goby)
install(DIRECTORY ${goby_INC_DIR}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/include)


## copy to goby/include
file(GLOB_RECURSE INCLUDE_FILES RELATIVE ${goby_SRC_DIR} src/*.h src/*.proto)
foreach(I ${INCLUDE_FILES})
  configure_file(${goby_SRC_DIR}/${I} ${goby_INC_DIR}/goby/${I} COPYONLY)
endforeach()

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${goby_INC_DIR}/goby)


## let cmake know where the headers are
include_directories(${goby_INC_DIR})

## for MAC OS X 
if(${APPLE})
  ## MacPorts
  include_directories(/opt/local/include)
  link_directories(/opt/local/lib)
  ## Fink
  include_directories(/sw/include)
  link_directories(/sw/lib)
endif()

## start adding subdirectories
add_subdirectory(src)

