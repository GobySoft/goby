option(build_pAcommsHandler "Build pAcommsHandler" ON)
option(build_pTranslator "Build pTranslator" ON)

if(build_pAcommsHandler)
  add_subdirectory(pAcommsHandler) # boost, xercesc, asio
endif()

if(build_pTranslator)
  add_subdirectory(pTranslator) 
endif()


add_subdirectory(dccl_xml_to_dccl_proto)
add_subdirectory(moos_gateway_g)
add_subdirectory(alog_to_goby_db)
add_subdirectory(goby_db_to_alog)

# broken
# add_subdirectory(analyze_dccl_xml)
