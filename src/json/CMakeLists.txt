set(ASCEND_FORGE_YYJSON_C "${CMAKE_SOURCE_DIR}/third_party/yyjson/yyjson.c")

target_sources(${PROJECT_NAME}_primitives PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/json.c
  ${ASCEND_FORGE_YYJSON_C}
)

target_include_directories(${PROJECT_NAME}_primitives PRIVATE
  ${CMAKE_SOURCE_DIR}/third_party/yyjson
)

# The vendored reader is larger than the warning set is worth rewriting.
# First-party json.c keeps warnings as errors. Hide yyjson's symbols.
# Source file properties are visible to the target only from the directory
# that created it. arena/ owns the primitives target.
if(MSVC)
  set_source_files_properties(${ASCEND_FORGE_YYJSON_C}
    TARGET_DIRECTORY ${PROJECT_NAME}_primitives
    PROPERTIES COMPILE_OPTIONS "/W0;/WX-"
  )
else()
  set_source_files_properties(${ASCEND_FORGE_YYJSON_C}
    TARGET_DIRECTORY ${PROJECT_NAME}_primitives
    PROPERTIES COMPILE_OPTIONS "-w;-fvisibility=hidden"
  )
endif()
