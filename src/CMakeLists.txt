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
  message(WARNING "missing optional external library: ${lib_name}. Goby will function without the support of this library, but some functionality or libraries will be not be available. See the goby/DEPENDENCIES file for more information.")
endmacro()

## find external packages, libraries, and headers for the entire goby project
# mandatory
## boost
goby_find_required_package(Boost 1.36.0 COMPONENTS system date_time thread program_options filesystem signals)
include_directories(${Boost_INCLUDE_DIRS})

goby_find_required_package(ProtobufGoby)
include_directories(${PROTOBUF_INCLUDE_DIRS})

# optional
## cryptopp
find_package(Cryptopp)
if(CRYPTOPP_FOUND)
  add_definitions(-DHAS_CRYPTOPP)
else()
  missing_optional_library(Cryptopp)
endif()  

## ncurses
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses)
if(CURSES_FOUND)
  add_definitions(-DHAS_NCURSES)
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
if(build_common)
  add_subdirectory(common)

  option(build_util "Build the Goby Utility library and applications (requires goby-common)" ON)
  if(build_util)
    add_subdirectory(util)

    option(build_acomms "Build the Goby Acoustic Communications library and applications (requires goby-util)" ON)
    if(build_acomms)
      add_subdirectory(acomms)

      
      find_package(ZeroMQ)
      if(ZEROMQ_FOUND)
        include_directories(${ZEROMQ_INCLUDE_DIRS})
        option(build_core "Build the Goby Core libraries and applications" ON)  
      else()
        missing_optional_library(ZeroMQ)
        option(build_core "Build the Goby Core libraries and applications" OFF) 
      endif()

      if(build_core)
        add_subdirectory(core)

        # MOOS requires core, so don't try to build if core is not built
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
      endif(build_core)
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
