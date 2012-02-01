option(build_pAcommsHandler "Build pAcommsHandler" ON)
option(build_pTranslator "Build pTranslator" ON)

if(build_pAcommsHandler)
  add_subdirectory(pAcommsHandler) # boost, xercesc, asio
endif()

if(build_pTranslator)
  add_subdirectory(pTranslator) 
endif()

if(enable_v1_transitional)
  add_subdirectory(dccl_xml_to_dccl_proto)
endif()

if(enable_zeromq)
  add_subdirectory(moos_gateway_g)
endif()
