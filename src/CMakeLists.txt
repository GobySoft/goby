## set up function for external libraries errors
# e.g. Boost libboost-dev 1.36 REQUIRED ... 
macro(goby_find_required_package package_name)
  find_package(${package_name} ${ARGN})
  string(TOUPPER ${package_name} package_name_upper)
  if(NOT ${package_name_upper}_FOUND AND NOT ${package_name}_FOUND)
    message(FATAL_ERROR "missing required external library: ${package_name}. Please read the goby/DEPENDENCIES file to resolve.")
  endif()
endmacro()
  
macro(missing_optional_library lib_name)
  message(WARNING "missing optional external library: ${lib_name}. Goby will function without the support of this library. See http://gobysoft/doc for more information.")
endmacro()

## find external packages, libraries, and headers for the entire goby project
# mandatory
## boost
goby_find_required_package(Boost 1.36.0 COMPONENTS system date_time thread regex program_options signals filesystem)
include_directories(${Boost_INCLUDE_DIRS})

goby_find_required_package(ProtobufGoby)

## ncurses
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses)
if(CURSES_FOUND)
  add_definitions(-DHAS_NCURSES)
else()
  missing_optional_library(CURSES)
endif()
mark_as_advanced(CURSES_CURSES_H_PATH CURSES_FORM_LIBRARY CURSES_HAVE_CURSES_H)


## set flags
option(build_util "Build the Goby Utility libraries and applications" ON)
option(build_acomms "Build the Goby Acoustic Communications libraries and applications" ON)
option(build_core "Build the Goby Core libraries and applications" ON)  
option(build_protobuf "Build the Goby Google Protocol buffers library" ON)  

find_package(MOOS)
if(MOOS_FOUND)
  include_directories(${MOOS_INCLUDE_DIRS})
  add_definitions(-DHAS_MOOS)
  option(build_moos "Build the Acoustic Communications MOOS apps (pAcommsHandler, etc.)" ON)
else()
  missing_optional_library(MOOS)
  option(build_moos "Build the Acoustic Communications MOOS apps (pAcommsHandler, etc.)" OFF)
endif()

if(build_util)
  add_subdirectory(util)
endif()

if(build_acomms)
  add_subdirectory(acomms)
endif()

if(build_core)
  add_subdirectory(core)
endif()

if(build_protobuf)
  add_subdirectory(protobuf)
endif()


if(build_moos)
  add_subdirectory(moos)
endif()

# latex, for documentation
find_package(LATEX)

option(build_doc "Build documentation (requires Doxygen, LaTeX, XeLaTeX, and certain fonts)" false)

if(build_doc)
  add_subdirectory(doc)
endif()

