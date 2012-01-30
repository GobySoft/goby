get_filename_component(dccl_examples_dir ./ ABSOLUTE)
add_definitions(-DDCCL_EXAMPLES_DIR="${dccl_examples_dir}")


if(enable_goby_v1_transitional_support)
  add_subdirectory(transitional1)
endif()

add_subdirectory(translator1)
