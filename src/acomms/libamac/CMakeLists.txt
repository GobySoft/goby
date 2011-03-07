file(GLOB amac_src *.cpp)
add_library(goby_amac SHARED ${amac_src})
target_link_libraries(goby_amac goby_logger ${Boost_LIBRARIES}  goby_protobuf)
set_target_properties(goby_amac PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")


