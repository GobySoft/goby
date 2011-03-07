file(GLOB SRC *.cpp)
add_library(goby_dbo SHARED ${SRC})
target_link_libraries(goby_dbo ${Boost_LIBRARIES} wtdbo wtdbosqlite3)
set_target_properties(goby_dbo PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")
