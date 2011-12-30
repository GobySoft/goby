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

add_subdirectory(database)
