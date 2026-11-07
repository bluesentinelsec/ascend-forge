# Unit tests — one subdirectory per component under test.
# Browser builds run the wasm test page instead of the native CTest suites,
# and they run the same arena checks as a second wasm page.
if(EMSCRIPTEN)
  add_subdirectory(web)
  if(ASCEND_FORGE_BUILD_PRIMITIVES)
    add_subdirectory(arena)
  endif()
  return()
endif()

add_subdirectory(version)
if(ASCEND_FORGE_BUILD_PRIMITIVES)
  add_subdirectory(arena)
endif()
