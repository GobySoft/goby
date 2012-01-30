find_path(Wt_INCLUDE_DIR Wt/WObject)

find_library(Wt_LIBRARY NAMES wt DOC "The Wt C++ Web library")

find_library(Wt_DBO_LIBRARY NAMES wtdbo DOC "The Wt Database Objects library")

find_library(Wt_DBO_SQLITE3_LIBRARY NAMES wtdbosqlite3
  DOC "The Wt Database Objects SQLITE3 binding library")

find_library(Wt_HTTP_LIBRARY NAMES wthttp
  DOC "The Wt HTTP binding library")


mark_as_advanced(
  Wt_INCLUDE_DIR 
  Wt_LIBRARY 
  Wt_DBO_LIBRARY 
  Wt_DBO_SQLITE3_LIBRARY
  Wt_HTTP_LIBRARY
 )

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Wt DEFAULT_MSG
   Wt_INCLUDE_DIR
   Wt_LIBRARY 
   Wt_DBO_LIBRARY
   Wt_DBO_SQLITE3_LIBRARY
   Wt_HTTP_LIBRARY
)

set(Wt_FOUND ${WT_FOUND}) 
if(Wt_FOUND)
  set(Wt_INCLUDE_DIRS ${Wt_INCLUDE_DIR})
  set(Wt_LIBRARIES    
    ${Wt_LIBRARY}
    ${Wt_DBO_LIBRARY}
    ${Wt_DBO_SQLITE3_LIBRARY}
    ${Wt_HTTP_LIBRARY})
endif()
