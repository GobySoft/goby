file(GLOB amac_src *.cpp)
add_library(goby_amac SHARED ${amac_src})
target_link_libraries(goby_amac goby_logger ${Boost_LIBRARIES} ${PROTOBUF_LIBRARIES} goby_protobuf)

if(build_acomms_examples)
  add_subdirectory(examples)
endif()
