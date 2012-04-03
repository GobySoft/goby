macro(TODAY RESULT)
  if (WIN32)
    execute_process(COMMAND "date" "/T" OUTPUT_VARIABLE ${RESULT})
    string(REGEX REPLACE "(..)/(..)/(....).*" "\\3.\\2.\\1"
      ${RESULT} ${${RESULT}})
  elseif(UNIX)
    execute_process(COMMAND "date" "+%d/%m/%Y" OUTPUT_VARIABLE ${RESULT})
    string(REGEX REPLACE "(..)/(..)/(....).*" "\\3.\\2.\\1"
      ${RESULT} ${${RESULT}})
  else (WIN32)
    message(SEND_ERROR "date not implemented")
    set(${RESULT} 000000)
  endif (WIN32)
endmacro(TODAY)
