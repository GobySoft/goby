file(GLOB driver_src *.cpp)
add_library(goby_modemdriver  ${driver_src})
target_link_libraries(goby_modemdriver goby_logger goby_linebasedcomms goby_protobuf)
set_target_properties(goby_modemdriver PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")
