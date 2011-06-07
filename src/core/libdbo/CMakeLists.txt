file(GLOB SRC *.cpp)
add_library(goby_dbo SHARED ${SRC})

set(GOBY_DBO_MAX_PROTOBUF_TYPES 16 CACHE INTEGER "Max number of compiled in Protobuf Types allowed in the goby_dbo library")

target_link_libraries(goby_dbo ${Boost_LIBRARIES} wtdbo wtdbosqlite3 goby_protobuf goby_logger goby_core)
if(MOOS_FOUND)
  target_link_libraries(goby_dbo ${MOOS_LIBRARIES})
endif()

set_target_properties(goby_dbo PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")
