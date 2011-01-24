file(GLOB driver_src *.cpp)
add_library(goby_modemdriver SHARED ${driver_src})
target_link_libraries(goby_modemdriver goby_logger goby_linebasedcomms)

if(build_acomms_examples)
  add_subdirectory(examples)
endif()
