get_filename_component(acomms_examples_dir ./ ABSOLUTE)
add_definitions(-DACOMMS_EXAMPLES_DIR="${acomms_examples_dir}")

add_subdirectory(chat)
add_subdirectory(libamac)
add_subdirectory(libdccl)
add_subdirectory(libqueue)
add_subdirectory(libmodemdriver)
