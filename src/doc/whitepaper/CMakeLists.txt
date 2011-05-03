add_custom_command(OUTPUT ${goby_SHARE_DIR}/doc/goby-whitepaper.pdf
  COMMAND xelatex
  ARGS ${CMAKE_CURRENT_BINARY_DIR}/whitepaper.tex -output-directory=${goby_SHARE_DIR}/doc
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/whitepaper.tex ${CMAKE_CURRENT_BINARY_DIR}/whitepaper.tex @ONLY)

add_custom_target(whitepaper-pdf ALL DEPENDS ${goby_SHARE_DIR}/doc/goby-whitepaper.pdf)
