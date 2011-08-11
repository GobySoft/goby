goby_find_required_package(MOOS)
include_directories(${MOOS_INCLUDE_DIRS})

option(build_pAcommsHandler "Build pAcommsHandler" ON)

if(build_pAcommsHandler)
	add_subdirectory(pAcommsHandler) # boost, xercesc, asio
endif()

add_subdirectory(libmoos_util) 
add_subdirectory(libtransitional) 
add_subdirectory(moos_gateway_g)

add_subdirectory(tools)
