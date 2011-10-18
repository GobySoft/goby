option(build_pAcommsHandler "Build pAcommsHandler" ON)

if(build_pAcommsHandler)
  add_subdirectory(pAcommsHandler) # boost, xercesc, asio
endif()

add_subdirectory(analyze_dccl_xml)
add_subdirectory(dccl_xml_to_dccl_proto)
