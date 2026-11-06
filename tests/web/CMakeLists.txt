# Browser-runtime tests: GoogleTest compiled to WebAssembly, run via emrun.
add_executable(ascend_forge_web_test web_test.cpp)
target_link_libraries(ascend_forge_web_test
  PRIVATE
    ${PROJECT_NAME}_lib
    GTest::gtest_main
)
set_target_properties(ascend_forge_web_test PROPERTIES SUFFIX ".html")
target_link_options(ascend_forge_web_test
  PRIVATE
    "--emrun"
    "SHELL:-sEXIT_RUNTIME=1"
    "SHELL:-sALLOW_MEMORY_GROWTH=1"
)
cppboot_set_project_warnings(ascend_forge_web_test)
