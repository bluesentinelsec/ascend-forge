add_executable(ascend_forge_atomic_test atomic_test.c)
set_target_properties(ascend_forge_atomic_test PROPERTIES
  C_STANDARD 17
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
)
target_link_libraries(ascend_forge_atomic_test
  PRIVATE
    ${PROJECT_NAME}::primitives
)
ascend_forge_set_c_warnings(ascend_forge_atomic_test)
if(EMSCRIPTEN)
  set_target_properties(ascend_forge_atomic_test PROPERTIES SUFFIX ".html")
  target_link_options(ascend_forge_atomic_test
    PRIVATE
      "--emrun"
      "SHELL:-sEXIT_RUNTIME=1"
      "SHELL:-sALLOW_MEMORY_GROWTH=1"
  )
else()
  add_test(NAME atomic COMMAND ascend_forge_atomic_test)
endif()
