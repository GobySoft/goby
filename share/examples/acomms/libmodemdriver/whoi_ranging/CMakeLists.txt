add_executable(whoi_ranging whoi_ranging.cpp)
target_link_libraries(whoi_ranging goby_modemdriver goby_protobuf protobuf crypto++ boost_system boost_signals)
