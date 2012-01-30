goby_find_required_package(MOOS)
find_package(Xerces)
if(Xerces_FOUND)
  include_directories(${Xerces_INCLUDE_DIRS})
  option(enable_goby_v1_transitional_support "Compile in MOOS support for Goby version 1 XML files" ON)
else()
  missing_optional_library(Xerces)
  option(enable_goby_v1_transitional_support "Compile in MOOS support for Goby version 1 XML files" OFF)
endif()  

if(enable_goby_v1_transitional_support)
  goby_find_required_package(Xerces)
  add_definitions(-DENABLE_GOBY_V1_TRANSITIONAL_SUPPORT)
endif()

include_directories(${MOOS_INCLUDE_DIRS})

file(GLOB PROTOS protobuf/*.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

set(SRC
   goby_moos_app.cpp
   modem_id_convert.cpp
   moos_translator.cpp
   transitional/message_val.cpp
   transitional/message_algorithms.cpp
)

if(enable_goby_v1_transitional_support)
  file(GLOB TRANSITIONAL_SRC transitional/*.cpp)
endif()

if(ZeroMQ_FOUND)
  set(ZEROMQ_SRC moos_node.cpp)
endif()

add_library(goby_moos ${SRC} ${ZEROMQ_SRC} ${TRANSITIONAL_SRC} ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(goby_moos
  goby_util
  goby_common 
  goby_acomms
  ${MOOS_LIBRARIES} ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY})

if(enable_goby_v1_transitional_support)
  target_link_libraries(goby_moos xerces-c)
endif()

set_target_properties(goby_moos PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")
