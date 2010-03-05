get_filename_component(dccl_examples_dir ./ ABSOLUTE)
add_definitions(-DDCCL_EXAMPLES_DIR="${dccl_examples_dir}")

add_subdirectory(plusnet)
add_subdirectory(dccl_simple)
add_subdirectory(test)
add_subdirectory(two_message)
add_subdirectory(delta)
