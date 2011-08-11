file(GLOB dccl_src *.cpp)
add_library(goby_dccl  ${dccl_src})

target_link_libraries(goby_dccl goby_logger goby_protobuf ${Cryptopp_LIBRARIES} ${Boost_LIBRARIES})

set_target_properties(goby_dccl PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")

if(build_acomms_tools)
  add_subdirectory(tools)
endif()

if(enable_testing)
  add_subdirectory(test)
endif()
