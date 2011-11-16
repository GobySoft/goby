option(build_pAcommsHandler "Build pAcommsHandler" ON)

if(build_pAcommsHandler)
  add_subdirectory(pAcommsHandler) # boost, xercesc, asio
endif()

add_subdirectory(analyze_dccl_xml)
add_subdirectory(dccl_xml_to_dccl_proto)
add_subdirectory(moos_gateway_g)
add_subdirectory(alog_to_goby_db)
add_subdirectory(goby_db_to_alog)

