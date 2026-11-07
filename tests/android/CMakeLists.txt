cmake_minimum_required(VERSION 3.22.1)
project(ascend_forge_android_test LANGUAGES C)

find_package(ascend_forge REQUIRED CONFIG)

add_library(ascend_forge_android_test SHARED
  android_test.c
  "${CMAKE_CURRENT_SOURCE_DIR}/../arena/arena_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../utf8/utf8_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../result/result_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../io/io_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../json/json_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../slice/slice_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../atomic/atomic_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../thread/thread_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../time/time_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../index/index_test.c"
)
target_compile_features(ascend_forge_android_test PRIVATE c_std_17)
target_compile_definitions(ascend_forge_android_test PRIVATE
  ASCEND_FORGE_ARENA_TESTS_NO_MAIN
  ASCEND_FORGE_UTF8_TESTS_NO_MAIN
  ASCEND_FORGE_RESULT_TESTS_NO_MAIN
  ASCEND_FORGE_IO_TESTS_NO_MAIN
  ASCEND_FORGE_JSON_TESTS_NO_MAIN
  ASCEND_FORGE_SLICE_TESTS_NO_MAIN
  ASCEND_FORGE_ATOMIC_TESTS_NO_MAIN
  ASCEND_FORGE_THREAD_TESTS_NO_MAIN
  ASCEND_FORGE_TIME_TESTS_NO_MAIN
  ASCEND_FORGE_INDEX_TESTS_NO_MAIN
)
target_include_directories(ascend_forge_android_test PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
  "${CMAKE_CURRENT_SOURCE_DIR}/../arena"
  "${CMAKE_CURRENT_SOURCE_DIR}/../utf8"
  "${CMAKE_CURRENT_SOURCE_DIR}/../result"
  "${CMAKE_CURRENT_SOURCE_DIR}/../io"
  "${CMAKE_CURRENT_SOURCE_DIR}/../json"
  "${CMAKE_CURRENT_SOURCE_DIR}/../slice"
  "${CMAKE_CURRENT_SOURCE_DIR}/../atomic"
  "${CMAKE_CURRENT_SOURCE_DIR}/../thread"
  "${CMAKE_CURRENT_SOURCE_DIR}/../time"
  "${CMAKE_CURRENT_SOURCE_DIR}/../index"
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
  "${CMAKE_CURRENT_SOURCE_DIR}/../result/result_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../io/io_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../json/json_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../slice/slice_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../atomic/atomic_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../thread/thread_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../time/time_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../index/index_test.c"
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
