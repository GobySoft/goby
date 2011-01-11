## find a package manager
find_program(aptitude_FOUND aptitude)
mark_as_advanced(aptitude_FOUND)


## set up function for external libraries errors
# e.g. Boost libboost-dev 1.36 REQUIRED ... 
macro(goby_find_required_package package_name apt_name)
  message("${package_name} ${ARGN}")
  find_package(${package_name} ${ARGN})

  if(NOT ${package_name}_FOUND)
#    message("missing required external library: ${package_name}.")
#    if(aptitude_FOUND)
#      message("Attempting to install using ${apt_name} using aptitude: ")
#      execute_process(COMMAND sudo ${aptitude_FOUND} -y install ${apt_name} RESULT_VARIABLE package_INSTALLED)
#    else()
      set(package_INSTALLED 1)
#    endif()
    
    # error is package_INSTALLED != 0 (return code)
    if(package_INSTALLED)
      message(FATAL_ERROR "Could not install package automatically. Please read the goby/DEPENDENCIES file to resolve.")
    else()
#      find_package(${package_name} ${ARGN})
    endif()
  endif()
endmacro()
  
function(missing_optional_library lib_name)
  message(WARNING "missing optional external library: ${package_name}. Goby will function without the support of this library. See http://gobysoft/doc for more information.")
endfunction()

## find external packages, libraries, and headers for the entire goby project
# mandatory
## boost
goby_find_required_package(Boost "libboost-dev libboost-system-dev libboost-date-time-dev libboost libboost-thread-dev libboost-regex-dev libboost-program-options-dev libboost-signals-dev libfilesystem-dev" 1.36.0 REQUIRED system date_time thread regex program_options signals filesystem)
include_directories(${Boost_INCLUDE_DIRS})

## ncurses
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses)
if(CURSES_FOUND)
  add_definitions(-DHAS_NCURSES)
endif()

if(NOT CURSES_FOUND)
  missing_optional_library(CURSES)
endif()
mark_as_advanced(CURSES_CURSES_H_PATH CURSES_FORM_LIBRARY CURSES_HAVE_CURSES_H)

## set flags
option(build_util "Build the Goby Utility libraries and applications" ON)
option(build_acomms "Build the Goby Acoustic Communications libraries and applications" ON)
option(build_core "Build the Goby Core libraries and applications" ON)  
option(build_test "Build test applications" OFF)

if(build_util)
  add_subdirectory(util)
endif()

if(build_acomms)
  add_subdirectory(acomms)
endif()

if(build_core)
  add_subdirectory(core)
endif()
