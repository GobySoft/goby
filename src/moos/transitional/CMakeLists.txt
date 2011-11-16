goby_find_required_package(Xerces)

file(GLOB transitional_src *.cpp)
add_library(goby_transitional  ${transitional_src})

target_link_libraries(goby_transitional goby_acomms xerces-c)
set_target_properties(goby_transitional PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")
