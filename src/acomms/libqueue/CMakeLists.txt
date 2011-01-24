file(GLOB queue_src *.cpp)
add_library(goby_queue SHARED ${queue_src})
target_link_libraries(goby_queue goby_logger xerces-c goby_dccl)

if(build_acomms_examples)
  add_subdirectory(examples)
endif()
