add_executable(ascend_forge_arena_test arena_test.c)
set_target_properties(ascend_forge_arena_test PROPERTIES
  C_STANDARD 17
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
)
target_link_libraries(ascend_forge_arena_test
  PRIVATE
    ${PROJECT_NAME}::primitives
)
ascend_forge_set_c_warnings(ascend_forge_arena_test)
add_test(NAME arena COMMAND ascend_forge_arena_test)
