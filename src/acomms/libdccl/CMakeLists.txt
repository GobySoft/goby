file(GLOB dccl_src *.cpp)
add_library(goby_dccl SHARED ${dccl_src})

## crytopp
goby_find_required_package(Cryptopp "libcryptopp-dev")

# icuuc needed by xerces
target_link_libraries(goby_dccl goby_logger goby_protobuf ${Xerces_LIBRARIES} icuuc ${Cryptopp_LIBRARIES} ${Boost_LIBRARIES})
set_target_properties(goby_dccl PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")

if(build_acomms_tools)
  add_subdirectory(tools)
endif()
