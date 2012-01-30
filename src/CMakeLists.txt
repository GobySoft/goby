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
  message(WARNING "missing optional external library: ${lib_name}. Goby will function without the support of this library, but some functionality will be not be available.")
endmacro()

## find external packages, libraries, and headers for the entire goby project
# mandatory
## boost
goby_find_required_package(Boost 1.36.0 COMPONENTS system date_time thread program_options filesystem signals regex)
include_directories(${Boost_INCLUDE_DIRS})

goby_find_required_package(ProtobufGoby)
include_directories(${PROTOBUF_INCLUDE_DIRS})

# optional
## cryptopp
find_package(Cryptopp)
if(CRYPTOPP_FOUND)
  add_definitions(-DHAS_CRYPTOPP)
  include_directories(${Cryptopp_INCLUDE_DIRS})
else()
  missing_optional_library(Cryptopp)
endif()  

## ncurses
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses)
if(CURSES_FOUND)
  add_definitions(-DHAS_NCURSES)
  include_directories(${CURSES_INCLUDE_DIR})
else()
  missing_optional_library(CURSES)
endif()
mark_as_advanced(CURSES_CURSES_H_PATH CURSES_FORM_LIBRARY CURSES_HAVE_CURSES_H)

option(workaround_2_4_kernel "Apply workarounds for Linux 2.4 kernel. Leave OFF for newer (e.g. 2.6) kernels" OFF)
if(workaround_2_4_kernel)
  add_definitions(-DBOOST_ASIO_DISABLE_EPOLL)
endif()


## set flags
option(build_common "Build the Goby Common library" ON)  

# module dependencies
# you can optionally build up to any point in this chain
# common -> util -> acomms -> moos

find_package(ZeroMQ)
if(ZEROMQ_FOUND)
  include_directories(${ZEROMQ_INCLUDE_DIRS})
else()
  missing_optional_library(ZeroMQ)
endif()

find_package(WtGoby)
# find resources directory
find_path(Wt_RESOURCES_DIR_BASE share/Wt/resources/plus.gif /usr /usr/local)
if(Wt_FOUND AND Wt_RESOURCES_DIR_BASE)
  include_directories(${Wt_INCLUDE_DIR})
  set(Wt_RESOURCES_DIR "${Wt_RESOURCES_DIR_BASE}/share/Wt/resources")
  message(${Wt_RESOURCES_DIR})

  add_custom_target(wt_link ALL DEPENDS WtResourcesLink)
  add_custom_command(OUTPUT WtResourcesLink COMMAND ${CMAKE_COMMAND} -E create_symlink ${Wt_RESOURCES_DIR} ${goby_SHARE_DIR}/liaison/resources)
else()
  missing_optional_library(Wt)
endif()  


if(build_common)
  add_subdirectory(common)

  option(build_util "Build the Goby Utility library and applications (requires goby-common)" ON)
  if(build_util)
    add_subdirectory(util)

    option(build_acomms "Build the Goby Acoustic Communications library and applications (requires goby-util)" ON)
    if(build_acomms)
      add_subdirectory(acomms)

  
        find_package(MOOS)
        if(MOOS_FOUND)
          include_directories(${MOOS_INCLUDE_DIRS})
          add_definitions(-DHAS_MOOS)
          option(build_moos "Build the Acoustic Communications MOOS apps (pAcommsHandler, etc.)" ON)
        else()
          missing_optional_library(MOOS)
          option(build_moos "Build the Acoustic Communications MOOS apps (pAcommsHandler, etc.)" OFF)
        endif()

        if(build_moos)
          add_subdirectory(moos)
        endif(build_moos)
    endif(build_acomms)
  endif(build_util)
endif(build_common)




# latex, for documentation
find_package(LATEX)

option(build_doc "Build documentation (requires Doxygen, LaTeX, XeLaTeX, and certain fonts)" OFF)

if(build_doc)
  add_subdirectory(doc)
endif()

if(enable_testing)
  add_subdirectory(test)
endif()


option(build_apps "Build applications (as well as libraries)" ON)

if(build_apps)
  add_subdirectory(apps)
endif()
