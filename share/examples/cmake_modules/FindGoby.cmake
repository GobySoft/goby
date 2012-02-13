set(GOBY_ROOT_DIR "GOBY_ROOT_DIR-NOTFOUND" CACHE STRING "Path to the root of Goby, e.g. /home/me/goby")

#
# Find include directory
# 
find_path(GOBY_INCLUDE_DIR goby/version.h
  PATHS ${CMAKE_SOURCE_DIR}/../goby ${CMAKE_SOURCE_DIR}/../../goby ${CMAKE_SOURCE_DIR}/../..  ${GOBY_ROOT_DIR}
  PATH_SUFFIXES include)
mark_as_advanced(GOBY_INCLUDE_DIR)

get_filename_component(GOBY_DIR ${GOBY_INCLUDE_DIR}/../ ABSOLUTE)
set(GOBY_LIBRARY_PATH "${GOBY_DIR}/lib")

message("Using Goby in ${GOBY_DIR}")

#
# Find libraries
# 

function(find_goby_library OUTPUT_VARIABLE LIBRARY_NAME GOBY_DIR)
  find_library(${OUTPUT_VARIABLE} NAMES ${LIBRARY_NAME}
    PATHS ${GOBY_DIR}/lib)

  set(OUTPUT_VARIABLE_CONTENTS ${${OUTPUT_VARIABLE}})
  if(OUTPUT_VARIABLE_CONTENTS)
    #message("OUTPUT_VARIABLE: ${OUTPUT_VARIABLE}")
    #message("OUTPUT_VARIABLE_CONTENTS: ${OUTPUT_VARIABLE_CONTENTS}")
    
    # this forces CMake to explicitly add to the linker the location
    # of the library that we just found
    add_library(${OUTPUT_VARIABLE} SHARED IMPORTED)
    set_target_properties(${OUTPUT_VARIABLE} PROPERTIES IMPORTED_LOCATION ${OUTPUT_VARIABLE_CONTENTS})
    add_library(${LIBRARY_NAME} SHARED IMPORTED)
    set_target_properties(${LIBRARY_NAME} PROPERTIES IMPORTED_LOCATION ${OUTPUT_VARIABLE_CONTENTS})
  endif()
    
  mark_as_advanced(${OUTPUT_VARIABLE})
  set(${OUTPUT_VARIABLE} PARENT_SCOPE)
endfunction()

find_goby_library(GOBY_ACOMMS_LIBRARY goby_acomms ${GOBY_DIR})
find_goby_library(GOBY_UTIL_LIBRARY goby_util ${GOBY_DIR})
find_goby_library(GOBY_COMMON_LIBRARY goby_common ${GOBY_DIR})

#
# Standard find_package portion
#

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Goby "Could NOT find Goby. Set GOBY_ROOT_DIR to the path where Goby is on your system (e.g. > cmake -DGOBY_ROOT_DIR=/home/me/goby .)"
  GOBY_DIR 
  GOBY_INCLUDE_DIR
  )

if(GOBY_FOUND)
  set(GOBY_INCLUDE_DIRS ${GOBY_INCLUDE_DIR})
  set(GOBY_LIBRARIES 
    ${GOBY_ACOMMS_LIBRARY}
    ${GOBY_UTIL_LIBRARY}
    ${GOBY_COMMON_LIBRARY}
    )

  set(GOBY_ROOT_DIR "${GOBY_DIR}" CACHE STRING "Path to the root of Goby, e.g. /home/me/goby" FORCE)
else()

endif()
