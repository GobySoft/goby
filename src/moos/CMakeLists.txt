goby_find_required_package(MOOS)
include_directories(${MOOS_INCLUDE_DIRS})

add_subdirectory(libmoos_util) 
add_subdirectory(libtransitional) 
