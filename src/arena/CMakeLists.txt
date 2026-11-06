add_library(${PROJECT_NAME}_primitives STATIC arena.c)
add_library(${PROJECT_NAME}::primitives ALIAS ${PROJECT_NAME}_primitives)

set_target_properties(${PROJECT_NAME}_primitives PROPERTIES
  OUTPUT_NAME ascend_forge_primitives
  C_STANDARD 17
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
)

target_include_directories(${PROJECT_NAME}_primitives
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}_primitives
  PUBLIC
    SDL3::SDL3-static
)

ascend_forge_set_c_warnings(${PROJECT_NAME}_primitives)
