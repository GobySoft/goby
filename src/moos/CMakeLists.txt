goby_find_required_package(MOOS)
include_directories(${MOOS_INCLUDE_DIRS})

find_package(Xerces QUIET)
set(TRANSITIONAL_DOC_STRING "Compile in MOOS support for Goby version 1 XML files (requires libxerces-c-dev")
if(Xerces_FOUND)
  option(enable_v1_transitional ${TRANSITIONAL_DOC_STRING} ON)
else()
  option(enable_v1_transitional ${TRANSITIONAL_DOC_STRING} OFF)
endif()

if(enable_v1_transitional)
  goby_find_required_package(Xerces)
  include_directories(${Xerces_INCLUDE_DIRS})
  add_definitions(-DENABLE_GOBY_V1_TRANSITIONAL_SUPPORT)
endif()  


file(GLOB PROTOS protobuf/*.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

set(SRC
   goby_moos_app.cpp
   modem_id_convert.cpp
   moos_translator.cpp
   transitional/message_val.cpp
   transitional/message_algorithms.cpp
)

if(enable_v1_transitional)
  file(GLOB TRANSITIONAL_SRC transitional/*.cpp)
endif()

if(enable_zeromq)
  set(ZEROMQ_SRC moos_node.cpp)
endif()

add_library(goby_moos ${SRC} ${ZEROMQ_SRC} ${TRANSITIONAL_SRC} ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(goby_moos
  goby_util
  goby_common 
  goby_acomms
  ${MOOS_LIBRARIES} ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY})

if(enable_v1_transitional)
  target_link_libraries(goby_moos ${Xerces_LIBRARIES})
endif()

set_target_properties(goby_moos PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")
