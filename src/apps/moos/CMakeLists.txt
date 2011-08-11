add_subdirectory(alog_to_goby_db)
option(build_pAcommsHandler "Build pAcommsHandler" ON)

if(build_pAcommsHandler)
	add_subdirectory(pAcommsHandler) # boost, xercesc, asio
endif()

add_subdirectory(moos_gateway_g)
