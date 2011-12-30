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

# broken
# add_subdirectory(analyze_dccl_xml)
