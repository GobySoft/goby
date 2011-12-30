get_filename_component(dccl_examples_dir ./ ABSOLUTE)
add_definitions(-DDCCL_EXAMPLES_DIR="${dccl_examples_dir}")

# broken
# add_subdirectory(transitional1)

add_subdirectory(translator1)
