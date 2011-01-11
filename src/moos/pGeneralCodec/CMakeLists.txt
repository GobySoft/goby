file(GLOB SRC *.cpp)

add_executable(pGeneralCodec ${SRC})

target_link_libraries(pGeneralCodec
    m
    pthread
    tes_util
    xerces-c
    nurc_moosapp
    moos_dccl
    goby_queue
    ${MOOS_LIBRARIES}
    )
