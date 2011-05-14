goby_find_required_package(Xerces)

file(GLOB transitional_src *.cpp)
add_library(goby_transitional SHARED ${transitional_src})

target_link_libraries(goby_transitional goby_dccl xerces-c)
set_target_properties(goby_transitional PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

