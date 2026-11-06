cmake_minimum_required(VERSION 3.22.1)
project(ascend_forge_android_test LANGUAGES CXX)

find_package(ascend_forge REQUIRED CONFIG)

add_library(ascend_forge_android_test SHARED android_test.cpp)
target_compile_features(ascend_forge_android_test PRIVATE cxx_std_20)
target_compile_options(ascend_forge_android_test PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Werror
)
target_link_libraries(ascend_forge_android_test PRIVATE ascend_forge::ascend_forge log)
