find_path(Xerces_INCLUDE_DIR xercesc/sax2/SAX2XMLReader.hpp)

find_library(Xerces_LIBRARY NAMES xerces-c
  DOC "The Xerces-C XML parsing library" PATHS /usr/lib)

mark_as_advanced(Xerces_INCLUDE_DIR
  Xerces_LIBRARY)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Xerces DEFAULT_MSG
  Xerces_LIBRARY Xerces_INCLUDE_DIR)

set(Xerces_FOUND ${XERCES_FOUND}) 
if(Xerces_FOUND)
  set(Xerces_INCLUDE_DIRS ${Xerces_INCLUDE_DIR})
  set(Xerces_LIBRARIES    ${Xerces_LIBRARY})
endif()

