find_path(GOBY_INCLUDE_DIR goby/version.h
  PATHS ${CMAKE_SOURCE_DIR}/../../include ${CMAKE_SOURCE_DIR}/../goby/include)

find_library(GOBY_AMAC_LIBRARY NAMES goby_amac
  DOC "The Goby Acoustic MAC library"
  PATHS ${CMAKE_SOURCE_DIR}/../../lib ${CMAKE_SOURCE_DIR}/../goby/lib)

find_library(GOBY_DCCL_LIBRARY NAMES goby_dccl
  DOC "The Goby DCCL library"
  PATHS ${CMAKE_SOURCE_DIR}/../../lib ${CMAKE_SOURCE_DIR}/../goby/lib)

find_library(GOBY_MODEMDRIVER_LIBRARY NAMES goby_modemdriver
  DOC "The Goby Modem Driver library"
  PATHS ${CMAKE_SOURCE_DIR}/../../lib ${CMAKE_SOURCE_DIR}/../goby/lib)

find_library(GOBY_QUEUE_LIBRARY NAMES goby_queue
  DOC "The Goby Queue library"
  PATHS ${CMAKE_SOURCE_DIR}/../../lib ${CMAKE_SOURCE_DIR}/../goby/lib)

find_library(GOBY_CORE_LIBRARY NAMES goby_core
  DOC "The Goby Core library"
  PATHS ${CMAKE_SOURCE_DIR}/../../lib ${CMAKE_SOURCE_DIR}/../goby/lib)


find_library(GOBY_LOGGER_LIBRARY NAMES goby_logger
  DOC "The Goby Logger library"
  PATHS ${CMAKE_SOURCE_DIR}/../../lib ${CMAKE_SOURCE_DIR}/../goby/lib)

find_library(GOBY_LINEBASEDCOMMS_LIBRARY NAMES goby_linebasedcomms
  DOC "The Goby LineBasedComms library"
  PATHS ${CMAKE_SOURCE_DIR}/../../lib ${CMAKE_SOURCE_DIR}/../goby/lib)


mark_as_advanced(GOBY_INCLUDE_DIR GOBY_AMAC_LIBRARY GOBY_DCCL_LIBRARY GOBY_MODEMDRIVER_LIBRARY GOBY_QUEUE_LIBRARY GOBY_CORE_LIBRARY GOBY_LOGGER_LIBRARY GOBY_LINEBASEDCOMMS_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Goby DEFAULT_MSG
  GOBY_INCLUDE_DIR GOBY_AMAC_LIBRARY GOBY_DCCL_LIBRARY GOBY_MODEMDRIVER_LIBRARY GOBY_QUEUE_LIBRARY GOBY_CORE_LIBRARY GOBY_LOGGER_LIBRARY GOBY_LINEBASEDCOMMS_LIBRARY)

if(GOBY_FOUND)
  set(GOBY_INCLUDE_DIRS ${GOBY_INCLUDE_DIR})
  set(GOBY_LIBRARIES ${GOBY_LIBRARY} ${GOBY_AMAC_LIBRARY} ${GOBY_DCCL_LIBRARY} ${GOBY_MODEMDRIVER_LIBRARY} ${GOBY_QUEUE_LIBRARY} ${GOBY_CORE_LIBRARY} ${GOBY_LOGGER_LIBRARY} ${GOBY_LINEBASEDCOMMS_LIBRARY})

  get_filename_component(GOBY_LIBRARY_PATH ${GOBY_AMAC_LIBRARY} PATH)
endif()
