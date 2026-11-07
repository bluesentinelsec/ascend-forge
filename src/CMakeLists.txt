# Library implementation components.
# Each logical subdirectory owns a CMakeLists.txt that lists sources explicitly.
add_subdirectory(version)
if(ASCEND_FORGE_BUILD_PRIMITIVES)
  add_subdirectory(arena)
  add_subdirectory(utf8)
  add_subdirectory(result)
  add_subdirectory(io)
endif()
# Example for new code:
#   add_subdirectory(parser)

# Painfully obvious program entrypoint: src/main.cpp
# Skipped when this project is consumed via add_subdirectory / FetchContent
# unless ASCEND_FORGE_BUILD_APP=ON.
if(ASCEND_FORGE_BUILD_APP)
  add_executable(${PROJECT_NAME}_app main.cpp)
  set_target_properties(${PROJECT_NAME}_app PROPERTIES OUTPUT_NAME ascend-forge)
  target_link_libraries(${PROJECT_NAME}_app PRIVATE ${PROJECT_NAME}_lib)
  if(ASCEND_FORGE_WITH_CLI11)
    target_link_libraries(${PROJECT_NAME}_app PRIVATE CLI11::CLI11)
  endif()
  if(ASCEND_FORGE_WITH_JSON)
    target_link_libraries(${PROJECT_NAME}_app PRIVATE nlohmann_json::nlohmann_json)
  endif()
  if(ASCEND_FORGE_WITH_SPDLOG)
    target_link_libraries(${PROJECT_NAME}_app PRIVATE spdlog::spdlog)
  endif()
  cppboot_set_project_warnings(${PROJECT_NAME}_app)
endif()

# Android Prefab shared library (built by the android/ Gradle project).
if(ANDROID)
  add_subdirectory(android)
endif()

# Web canvas demo (Emscripten game loop; see src/web/).
if(EMSCRIPTEN)
  add_subdirectory(web)
endif()
