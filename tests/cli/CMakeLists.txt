add_executable(ascend_forge_cli_test cli_test.c)
set_target_properties(ascend_forge_cli_test PROPERTIES
  C_STANDARD 17
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
)
target_link_libraries(ascend_forge_cli_test
  PRIVATE
    ${PROJECT_NAME}::primitives
)
ascend_forge_set_c_warnings(ascend_forge_cli_test)
if(EMSCRIPTEN)
  set_target_properties(ascend_forge_cli_test PROPERTIES SUFFIX ".html")
  target_link_options(ascend_forge_cli_test
    PRIVATE
      "--emrun"
      "SHELL:-sEXIT_RUNTIME=1"
      "SHELL:-sALLOW_MEMORY_GROWTH=1"
  )
else()
  add_test(NAME cli COMMAND ascend_forge_cli_test)
endif()
