file(GLOB driver_src *.cpp)
add_library(goby_modemdriver SHARED ${driver_src})
target_link_libraries(goby_modemdriver goby_logger goby_linebasedcomms goby_protobuf crypto++)
set_target_properties(goby_modemdriver PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")

if(build_acomms_tools)
  add_subdirectory(tools)
endif()
