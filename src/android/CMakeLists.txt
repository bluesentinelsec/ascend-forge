# Android Prefab library. Gradle publishes this target as the `ascend_forge`
# module in the AAR. Whole-archive linking keeps every public C symbol
# while folding the static core library into one shared object.
# SDL ships src/hidapi/android/hid.cpp. This anchor stays C, and the shared
# library still links with the C++ driver so libc++ resolves operator new.
enable_language(CXX)
add_library(ascend_forge SHARED android_library.c)
set_target_properties(ascend_forge PROPERTIES LINKER_LANGUAGE CXX)

target_compile_features(ascend_forge PUBLIC c_std_17)
target_include_directories(ascend_forge
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${ASCEND_FORGE_GENERATED_INCLUDE_DIR}>
)

add_dependencies(ascend_forge ${PROJECT_NAME}_lib)
if(ASCEND_FORGE_BUILD_PRIMITIVES)
  # Whole-archive keeps Arena_* in the .so. Static SDL resolves those
  # references, including its Android JNI_OnLoad.
  add_dependencies(ascend_forge ${PROJECT_NAME}_primitives)
  target_link_options(ascend_forge
    PRIVATE
      "LINKER:--whole-archive,$<TARGET_FILE:${PROJECT_NAME}_lib>,$<TARGET_FILE:${PROJECT_NAME}_primitives>,--no-whole-archive"
  )
  target_link_libraries(ascend_forge PRIVATE SDL3::SDL3-static)
else()
  target_link_options(ascend_forge
    PRIVATE
      "LINKER:--whole-archive,$<TARGET_FILE:${PROJECT_NAME}_lib>,--no-whole-archive"
  )
endif()

set_target_properties(ascend_forge PROPERTIES
  OUTPUT_NAME ascend_forge
)

ascend_forge_set_c_warnings(ascend_forge)
