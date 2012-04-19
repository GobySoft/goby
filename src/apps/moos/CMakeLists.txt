
# for now, we need to be able to link in UDPDriver and PBDriver
# at some point, make these dynamically loadable
if(enable_zeromq AND build_pb)
  option(build_pAcommsHandler "Build pAcommsHandler" ON)
else()
  option(build_pAcommsHandler "Build pAcommsHandler" OFF)
endif()

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
