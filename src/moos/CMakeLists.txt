goby_find_required_package(MOOS)
include_directories(${MOOS_INCLUDE_DIRS})

option(build_iCommander "Build iCommander" ON)
option(build_pREMUSCodec "Build pREMUSCodecs" ON)
option(build_pAcommsHandler "Build pAcommsHandler" ON)

if(build_iCommander)
	add_subdirectory(iCommander)   # boost, ncurses, cdk
endif()

if(build_pAcommsHandler)
	add_subdirectory(pAcommsHandler) # boost, xercesc, asio
endif()

if(build_pREMUSCodec)
	add_subdirectory(pREMUSCodec)
endif()

add_subdirectory(libmoos_util) 
