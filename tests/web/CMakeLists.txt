# Browser-runtime checks, run via emrun.
add_executable(ascend_forge_web_test web_test.c)
set_target_properties(ascend_forge_web_test PROPERTIES
  C_STANDARD 17
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
  SUFFIX ".html"
)
target_link_libraries(ascend_forge_web_test
  PRIVATE
    ${PROJECT_NAME}_lib
)
target_link_options(ascend_forge_web_test
  PRIVATE
    "--emrun"
    "SHELL:-sEXIT_RUNTIME=1"
    "SHELL:-sALLOW_MEMORY_GROWTH=1"
)
ascend_forge_set_c_warnings(ascend_forge_web_test)
