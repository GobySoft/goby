if(ZEROMQ_FOUND)
  add_subdirectory(zero_mq_node1)
  add_subdirectory(zero_mq_node2)
  add_subdirectory(zero_mq_node3)
endif()
# there's a problem with this test failing based on clock parameters
#add_subdirectory(zero_mq_node4)

