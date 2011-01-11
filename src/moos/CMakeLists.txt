goby_find_required_package(MOOS "moos")
include_directories(${MOOS_INCLUDE_DIRS})


add_subdirectory(iCommander)   # boost, ncurses, cdk
add_subdirectory(pAcommsHandler) # boost, xercesc, asio
add_subdirectory(lib_moos_dccl) # boost
add_subdirectory(pREMUSCodec)
add_subdirectory(pGeneralCodec) # requires boost and xercesc
add_subdirectory(lib_tes_util) 
add_subdirectory(lib_nurc_moosapp) 
