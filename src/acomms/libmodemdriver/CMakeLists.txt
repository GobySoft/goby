file(GLOB driver_src *.cpp)
add_library(goby_modemdriver SHARED ${driver_src})
target_link_libraries(goby_modemdriver goby_logger goby_linebasedcomms goby_protobuf crypto++ ${PROTOBUF_LIBRARIES})

if(build_acomms_examples)
  add_subdirectory(examples)
endif()
