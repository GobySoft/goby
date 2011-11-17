#
# Ubuntu patched this compiler to hell
# gcc (Ubuntu/Linaro 4.4.4-14ubuntu5) 4.4.5
#
exec_program(${CMAKE_CXX_COMPILER}
  ARGS --version
  OUTPUT_VARIABLE GCC_COMPILER_VERSION
)

if(GCC_COMPILER_VERSION MATCHES ".*4\\.4\\.4\\-14ubuntu5.*")
  message(" ")
  message("    !!!!! WARNING Your compiler is BUGGY. !!!!! ")
  message(" ")
  message("    If possible, upgrade your compiler to e.g. g++ 4.5:")
  message(" ")
  message("       $ sudo apt-get install g++-4.5")
  message(" ")
  message("    And build using that compiler cmake -DCMAKE_CXX_COMPILER=g++-4.5")
  message(" ")
  message("    we will now disable all assertions as a work around, by")
  message("    building using -DNDEBUG. You will need to define this")
  message("    also for programs built using Goby")
  message(" ")
  add_definitions(-DNDEBUG)
endif(GCC_COMPILER_VERSION MATCHES ".*4\\.4\\.4\\-14ubuntu5.*")


file(GLOB PROTOS protobuf/*.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

file(GLOB DBO_SRC dbo/*.cpp)
file(GLOB PB_SRC *.cpp)

add_library(goby_pb ${DBO_SRC} ${PB_SRC} ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(goby_pb ${Boost_LIBRARIES} pthread goby_common goby_util ${ZeroMQ_LIBRARIES} ${PROTOBUF_LIBRARIES} wtdbo wtdbosqlite3)


if(NOT ${APPLE})
  target_link_libraries(goby_pb rt) 
endif()

set_target_properties(goby_pb PROPERTIES VERSION "${GOBY_VERSION}" SOVERSION "${GOBY_SOVERSION}")
