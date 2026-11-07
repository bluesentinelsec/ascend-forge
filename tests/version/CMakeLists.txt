add_executable(ascend_forge_version_test version_test.c)
set_target_properties(ascend_forge_version_test PROPERTIES
  C_STANDARD 17
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
)
target_link_libraries(ascend_forge_version_test
  PRIVATE
    ${PROJECT_NAME}_lib
)
ascend_forge_set_c_warnings(ascend_forge_version_test)
add_test(NAME version COMMAND ascend_forge_version_test)
