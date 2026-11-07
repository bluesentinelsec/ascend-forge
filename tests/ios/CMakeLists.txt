cmake_minimum_required(VERSION 3.28)

project(ascend_forge_ios_package_test LANGUAGES C CXX OBJCXX)

if(NOT IOS)
  message(FATAL_ERROR "The ascend-forge iOS package test must be configured with CMAKE_SYSTEM_NAME=iOS")
endif()

set(ASCEND_FORGE_XCFRAMEWORK "" CACHE PATH "Path to the packaged ascend-forge.xcframework")
set(ASCEND_FORGE_EXPECTED_VERSION "" CACHE STRING "Version expected from the packaged library")

if(NOT IS_DIRECTORY "${ASCEND_FORGE_XCFRAMEWORK}")
  message(FATAL_ERROR "Set ASCEND_FORGE_XCFRAMEWORK to the packaged ascend-forge.xcframework directory")
endif()
if(ASCEND_FORGE_EXPECTED_VERSION STREQUAL "")
  message(FATAL_ERROR "Set ASCEND_FORGE_EXPECTED_VERSION to the root VERSION value")
endif()

if(CMAKE_OSX_SYSROOT MATCHES "[Ss]imulator")
  set(ASCEND_FORGE_XCFRAMEWORK_SLICE "ios-arm64_x86_64-simulator")
else()
  set(ASCEND_FORGE_XCFRAMEWORK_SLICE "ios-arm64")
endif()
set(ASCEND_FORGE_PACKAGE_HEADERS
  "${ASCEND_FORGE_XCFRAMEWORK}/${ASCEND_FORGE_XCFRAMEWORK_SLICE}/Headers"
)

add_executable(ascend_forge_ios_test MACOSX_BUNDLE
  test_main.mm
  "${CMAKE_CURRENT_SOURCE_DIR}/../arena/arena_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../utf8/utf8_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../result/result_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../io/io_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../json/json_test.c"
)
target_compile_features(ascend_forge_ios_test PRIVATE cxx_std_20 c_std_17)
set_source_files_properties(test_main.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
set_source_files_properties(
  "${CMAKE_CURRENT_SOURCE_DIR}/../arena/arena_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../utf8/utf8_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../result/result_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../io/io_test.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../json/json_test.c"
  PROPERTIES COMPILE_OPTIONS
    "-Wall;-Wextra;-Wpedantic;-Werror;-Wconversion;-Wshadow;-Wcast-align;-Wunused"
)
target_compile_definitions(ascend_forge_ios_test PRIVATE
  ASCEND_FORGE_EXPECTED_VERSION="${ASCEND_FORGE_EXPECTED_VERSION}"
  ASCEND_FORGE_ARENA_TESTS_NO_MAIN
  ASCEND_FORGE_UTF8_TESTS_NO_MAIN
  ASCEND_FORGE_RESULT_TESTS_NO_MAIN
  ASCEND_FORGE_IO_TESTS_NO_MAIN
  ASCEND_FORGE_JSON_TESTS_NO_MAIN
)
target_include_directories(ascend_forge_ios_test PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
  "${CMAKE_CURRENT_SOURCE_DIR}/../arena"
  "${CMAKE_CURRENT_SOURCE_DIR}/../utf8"
  "${CMAKE_CURRENT_SOURCE_DIR}/../result"
  "${CMAKE_CURRENT_SOURCE_DIR}/../io"
  "${CMAKE_CURRENT_SOURCE_DIR}/../json"
)
target_include_directories(ascend_forge_ios_test SYSTEM PRIVATE
  "${ASCEND_FORGE_PACKAGE_HEADERS}"
)
# These are the system frameworks the pinned SDL 3.4.20 iOS static library
# references. -ObjC keeps SDL's Objective-C classes in the link.
target_link_libraries(ascend_forge_ios_test PRIVATE
  "${ASCEND_FORGE_XCFRAMEWORK}"
  "-ObjC"
  "-framework Foundation"
  "-framework UIKit"
  "-framework CoreVideo"
  "-framework CoreMedia"
  "-framework CoreAudio"
  "-framework AudioToolbox"
  "-framework AVFoundation"
  "-framework CoreMotion"
  "-framework GameController"
  "-framework CoreHaptics"
  "-framework CoreGraphics"
  "-framework QuartzCore"
  "-framework OpenGLES"
  "-framework Metal"
  "-framework CoreBluetooth"
)

set_target_properties(ascend_forge_ios_test PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
  MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.ascendforge.test"
  MACOSX_BUNDLE_BUNDLE_NAME "ascend-forge iOS Tests"
  MACOSX_BUNDLE_BUNDLE_VERSION "1"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "${ASCEND_FORGE_EXPECTED_VERSION}"
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.example.ascendforge.test"
  XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
  XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
  XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
)
