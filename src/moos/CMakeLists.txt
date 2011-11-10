goby_find_required_package(MOOS)
include_directories(${MOOS_INCLUDE_DIRS})

add_subdirectory(libtransitional) 

file(GLOB PROTOS *.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

file(GLOB SRC *.cpp *.c)
add_library(goby_moos  ${SRC} ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(goby_moos goby_util goby_protobuf ${MOOS_LIBRARIES} ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY})

set_target_properties(goby_moos PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")
