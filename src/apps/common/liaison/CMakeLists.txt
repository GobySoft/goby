
add_definitions(-DGOBY_LIAISON_COMPILED_DOCROOT="${goby_SHARE_DIR}/liaison")
add_definitions(-DGOBY_LIAISON_INSTALLED_DOCROOT="${CMAKE_INSTALL_PREFIX}/share/goby/liaison")


file(GLOB PROTOS *.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})

add_executable(goby_liaison
  liaison.cpp
  liaison_scope.cpp 
  liaison_home.cpp  
  liaison_commander.cpp
  liaison_wt_thread.cpp
  ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(goby_liaison
  ${Boost_LIBRARIES}
  ${Wt_LIBRARIES}
  pthread
  goby_common
  goby_moos
  goby_util
  )

