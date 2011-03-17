find_path(Cryptopp_INCLUDE_DIR cryptopp/aes.h)

find_library(Cryptopp_LIBRARY NAMES cryptopp
  DOC "The Cryptopp Encrpytion library")

mark_as_advanced(Cryptopp_INCLUDE_DIR
  Cryptopp_LIBRARY)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Cryptopp DEFAULT_MSG
  Cryptopp_LIBRARY Cryptopp_INCLUDE_DIR)

set(Cryptopp_FOUND  ${CRYPTOPP_FOUND})
if(Cryptopp_FOUND)
  set(Cryptopp_INCLUDE_DIRS ${Cryptopp_INCLUDE_DIR})
  set(Cryptopp_LIBRARIES    ${Cryptopp_LIBRARY})
endif()

