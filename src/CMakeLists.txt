# Library implementation components.
# Each logical subdirectory owns a CMakeLists.txt that lists sources explicitly.
add_subdirectory(version)
if(ASCEND_FORGE_BUILD_PRIMITIVES)
  add_subdirectory(arena)
  add_subdirectory(utf8)
  add_subdirectory(result)
  add_subdirectory(io)
  add_subdirectory(json)
  add_subdirectory(slice)
  add_subdirectory(atomic)
  add_subdirectory(thread)
  add_subdirectory(time)
endif()
# Example for new code:
#   add_subdirectory(parser)

# Program entrypoint: src/main.c
# Skipped when this project is consumed via add_subdirectory / FetchContent
# unless ASCEND_FORGE_BUILD_APP=ON.
if(ASCEND_FORGE_BUILD_APP)
  add_executable(${PROJECT_NAME}_app main.c)
  set_target_properties(${PROJECT_NAME}_app PROPERTIES
    OUTPUT_NAME ascend-forge
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
  )
  # Whole-archive keeps every SDL object in the host. A normal static link
  # drops objects main.c does not reference, and a plugin would not find them.
  # The core archive stays unlinked from SDL so install(EXPORT) does not
  # require the unexported SDL3-static target.
  target_link_libraries(${PROJECT_NAME}_app
    PRIVATE
      ${PROJECT_NAME}_lib
      $<LINK_LIBRARY:WHOLE_ARCHIVE,SDL3::SDL3-static>
  )
  ascend_forge_set_c_warnings(${PROJECT_NAME}_app)
endif()

# Android Prefab shared library (built by the android/ Gradle project).
if(ANDROID)
  add_subdirectory(android)
endif()

# Web canvas demo (Emscripten game loop; see src/web/).
if(EMSCRIPTEN)
  add_subdirectory(web)
endif()
