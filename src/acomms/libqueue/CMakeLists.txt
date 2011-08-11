file(GLOB queue_src *.cpp)
add_library(goby_queue SHARED ${queue_src})
target_link_libraries(goby_queue goby_logger goby_protobuf goby_dccl ${Xerces_LIBRARIES})
set_target_properties(goby_queue PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")

if(enable_testing)
  add_subdirectory(test)
endif()
