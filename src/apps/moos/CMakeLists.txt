option(build_pAcommsHandler "Build pAcommsHandler" ON)
option(build_pTranslator "Build pTranslator" ON)

if(build_pAcommsHandler)
  add_subdirectory(pAcommsHandler) # boost, xercesc, asio
endif()

if(build_pTranslator)
  add_subdirectory(pTranslator) 
endif()



if(enable_goby_v1_transitional_support)
  add_subdirectory(dccl_xml_to_dccl_proto)
endif()

add_subdirectory(moos_gateway_g)
