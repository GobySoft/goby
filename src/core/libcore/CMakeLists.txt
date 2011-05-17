file(GLOB SRC *.cpp)
add_library(goby_core SHARED ${SRC})

target_link_libraries(goby_core ${Boost_LIBRARIES} pthread goby_protobuf goby_logger ${PROTOBUF_LIBRARIES})

if(NOT ${APPLE})
  target_link_libraries(goby_core rt) 
endif()

set_target_properties(goby_core PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")
