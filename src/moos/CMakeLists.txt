goby_find_required_package(MOOS)
include_directories(${MOOS_INCLUDE_DIRS})

add_subdirectory(iCommander)   # boost, ncurses, cdk
add_subdirectory(pAcommsHandler) # boost, xercesc, asio
add_subdirectory(pREMUSCodec)
add_subdirectory(libmoos_util) 
add_subdirectory(moos_gateway_g)