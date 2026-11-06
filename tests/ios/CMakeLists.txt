cmake_minimum_required(VERSION 3.28)

project(ascend_forge_ios_package_test LANGUAGES CXX OBJCXX)

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

add_executable(ascend_forge_ios_test MACOSX_BUNDLE test_main.mm)
target_compile_features(ascend_forge_ios_test PRIVATE cxx_std_20)
target_compile_options(ascend_forge_ios_test PRIVATE -fobjc-arc)
target_compile_definitions(ascend_forge_ios_test PRIVATE
  ASCEND_FORGE_EXPECTED_VERSION="${ASCEND_FORGE_EXPECTED_VERSION}"
)
target_link_libraries(ascend_forge_ios_test PRIVATE
  "${ASCEND_FORGE_XCFRAMEWORK}"
  "-framework Foundation"
  "-framework UIKit"
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
