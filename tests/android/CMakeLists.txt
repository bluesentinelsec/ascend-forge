cmake_minimum_required(VERSION 3.22.1)
project(ascend_forge_android_test LANGUAGES C CXX)

find_package(ascend_forge REQUIRED CONFIG)

add_library(ascend_forge_android_test SHARED
  android_test.cpp
  "${CMAKE_CURRENT_SOURCE_DIR}/../arena/arena_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../utf8/utf8_test.c"
)
target_compile_features(ascend_forge_android_test PRIVATE cxx_std_20 c_std_17)
target_compile_definitions(ascend_forge_android_test PRIVATE
  ASCEND_FORGE_ARENA_TESTS_NO_MAIN
  ASCEND_FORGE_UTF8_TESTS_NO_MAIN
)
target_include_directories(ascend_forge_android_test PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
  "${CMAKE_CURRENT_SOURCE_DIR}/../arena"
  "${CMAKE_CURRENT_SOURCE_DIR}/../utf8"
)
get_target_property(ASCEND_FORGE_PREFAB_INCLUDES ascend_forge::ascend_forge
  INTERFACE_INCLUDE_DIRECTORIES
)
if(ASCEND_FORGE_PREFAB_INCLUDES)
  target_include_directories(ascend_forge_android_test SYSTEM PRIVATE
    ${ASCEND_FORGE_PREFAB_INCLUDES}
  )
endif()
set_source_files_properties(
  "${CMAKE_CURRENT_SOURCE_DIR}/../arena/arena_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../utf8/utf8_test.c"
  PROPERTIES COMPILE_OPTIONS
    "-Wall;-Wextra;-Wpedantic;-Werror;-Wconversion;-Wshadow;-Wcast-align;-Wunused"
)
target_compile_options(ascend_forge_android_test PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Werror
)
target_link_libraries(ascend_forge_android_test PRIVATE ascend_forge::ascend_forge log)
