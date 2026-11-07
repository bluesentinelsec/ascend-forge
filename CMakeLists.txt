cmake_minimum_required(VERSION 3.20)

# ---------------------------------------------------------------------------
# Single source of truth for the package version: the VERSION file at repo root.
# Edit that file only; CMake, --version, tests, and release checks all consume it.
# ---------------------------------------------------------------------------
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" ASCEND_FORGE_VERSION_RAW
  LIMIT_COUNT 1
)
string(STRIP "${ASCEND_FORGE_VERSION_RAW}" ASCEND_FORGE_VERSION)
# Allow optional leading 'v' and ignore anything after '#' on the line.
string(REGEX REPLACE "^[vV]" "" ASCEND_FORGE_VERSION "${ASCEND_FORGE_VERSION}")
string(REGEX REPLACE "[ \t]*#.*" "" ASCEND_FORGE_VERSION "${ASCEND_FORGE_VERSION}")
string(STRIP "${ASCEND_FORGE_VERSION}" ASCEND_FORGE_VERSION)
if(NOT ASCEND_FORGE_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+([.-].*)?$")
  message(FATAL_ERROR
    "VERSION file must contain a semantic version like 0.1.0 "
    "(got '${ASCEND_FORGE_VERSION}')"
  )
endif()

project(ascend-forge
  VERSION ${ASCEND_FORGE_VERSION}
  DESCRIPTION "Plain C host on static SDL3"
  LANGUAGES C
)

# True when this project is the top-level CMake project (not add_subdirectory /
# FetchContent). Downstream consumers should not inherit app/tests/bench defaults.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(ASCEND_FORGE_IS_TOP_LEVEL ON)
else()
  set(ASCEND_FORGE_IS_TOP_LEVEL OFF)
endif()

# Exposed to version templates (configure_file @ONLY).
set(PROJECT_NAMESPACE "ascend_forge")
set(PROJECT_VERSION_STRING "${PROJECT_VERSION}")

# Top-level-only settings so embedding via add_subdirectory/FetchContent stays clean.
if(ASCEND_FORGE_IS_TOP_LEVEL)
  # Export a compilation database for clangd and other LSP tools.
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  # Put binaries under build/<config>/bin (and libs under lib/) so an executable
  # named like a component directory (e.g. project "calc" + src/calc/) never
  # collides with a source/build subdirectory path.
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

  # Default to Debug when the user does not pass CMAKE_BUILD_TYPE (single-config).
  if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
  endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
# Absolute-path includes: module-name include() resolves against
# CMAKE_MODULE_PATH, and when this project is embedded (add_subdirectory /
# FetchContent) the PARENT project's cmake/ is searched first — a same-named
# parent module would silently shadow ours.
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerWarnings.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Sanitizers.cmake")

# Kept so existing configure lines that pass these flags still match an option.
# The host does not fetch CLI11, nlohmann JSON, or spdlog.
option(ASCEND_FORGE_WITH_CLI11 "Unused. The C entrypoint does not fetch CLI11" OFF)
option(ASCEND_FORGE_WITH_JSON "Unused. JSON is the C Json_ primitive" OFF)
option(ASCEND_FORGE_WITH_SPDLOG "Unused. Logging is SDL_Log" OFF)

# When embedded, skip app/tests/benchmarks unless the consumer opts in. Platform
# package consumers receive the C++ library as a packaged artifact; repository
# tests run in the platform test application instead of CTest (browser tests
# under tests/web keep BUILD_TESTS meaningful for Emscripten).
set(ASCEND_FORGE_DEFAULT_BUILD_APP ${ASCEND_FORGE_IS_TOP_LEVEL})
set(ASCEND_FORGE_DEFAULT_BUILD_TESTS ${ASCEND_FORGE_IS_TOP_LEVEL})
set(ASCEND_FORGE_DEFAULT_BUILD_BENCHMARKS OFF)
if(ANDROID OR IOS)
  set(ASCEND_FORGE_DEFAULT_BUILD_APP OFF)
  set(ASCEND_FORGE_DEFAULT_BUILD_TESTS OFF)
endif()
if(EMSCRIPTEN)
  set(ASCEND_FORGE_DEFAULT_BUILD_APP OFF)
endif()
option(ASCEND_FORGE_BUILD_APP "Build the demo application executable" ${ASCEND_FORGE_DEFAULT_BUILD_APP})
option(ASCEND_FORGE_BUILD_TESTS "Build unit tests" ${ASCEND_FORGE_DEFAULT_BUILD_TESTS})
option(ASCEND_FORGE_BUILD_BENCHMARKS "Build benchmarks" ${ASCEND_FORGE_DEFAULT_BUILD_BENCHMARKS})
# HTML5 canvas demo page for top-level Emscripten builds (see src/web/).
option(ASCEND_FORGE_BUILD_WEB_DEMO "Build the web canvas demo (Emscripten only)" ${ASCEND_FORGE_IS_TOP_LEVEL})

# First-party C primitives. Every top-level platform links static SDL3,
# including Android, iOS, and Emscripten.
set(ASCEND_FORGE_DEFAULT_BUILD_PRIMITIVES ${ASCEND_FORGE_IS_TOP_LEVEL})
option(ASCEND_FORGE_BUILD_PRIMITIVES
  "Build the C primitive library (arena and later primitives)"
  ${ASCEND_FORGE_DEFAULT_BUILD_PRIMITIVES}
)

if(ANDROID AND (ASCEND_FORGE_BUILD_APP OR ASCEND_FORGE_BUILD_TESTS OR ASCEND_FORGE_BUILD_BENCHMARKS))
  message(FATAL_ERROR
    "Android builds provide the ascend_forge Prefab library; set ASCEND_FORGE_BUILD_APP, "
    "ASCEND_FORGE_BUILD_TESTS, and ASCEND_FORGE_BUILD_BENCHMARKS to OFF. "
    "Use the android Gradle project to build and run device tests."
  )
endif()

if(ANDROID AND (ASCEND_FORGE_WITH_CLI11 OR ASCEND_FORGE_WITH_JSON OR ASCEND_FORGE_WITH_SPDLOG))
  message(FATAL_ERROR
    "The Android Prefab AAR contains the dependency-free core API; set "
    "ASCEND_FORGE_WITH_CLI11, ASCEND_FORGE_WITH_JSON, and ASCEND_FORGE_WITH_SPDLOG to OFF"
  )
endif()
if(IOS AND (ASCEND_FORGE_BUILD_APP OR ASCEND_FORGE_BUILD_TESTS OR ASCEND_FORGE_BUILD_BENCHMARKS))
  message(FATAL_ERROR
    "iOS builds provide the ascend_forge XCFramework library; set ASCEND_FORGE_BUILD_APP, "
    "ASCEND_FORGE_BUILD_TESTS, and ASCEND_FORGE_BUILD_BENCHMARKS to OFF. "
    "Use the iOS package test application for Simulator tests."
  )
endif()

if(IOS AND (ASCEND_FORGE_WITH_CLI11 OR ASCEND_FORGE_WITH_JSON OR ASCEND_FORGE_WITH_SPDLOG))
  message(FATAL_ERROR
    "The iOS XCFramework contains the dependency-free core API; set "
    "ASCEND_FORGE_WITH_CLI11, ASCEND_FORGE_WITH_JSON, and ASCEND_FORGE_WITH_SPDLOG to OFF"
  )
endif()
if(EMSCRIPTEN AND (ASCEND_FORGE_BUILD_APP OR ASCEND_FORGE_BUILD_BENCHMARKS))
  message(FATAL_ERROR
    "Browser builds provide the static ascend_forge library, the web demo, and "
    "browser tests; set ASCEND_FORGE_BUILD_APP and ASCEND_FORGE_BUILD_BENCHMARKS to OFF"
  )
endif()
# ASan + UBSan for project targets (intended for Linux GCC/Clang; see make sanitizer).
option(ASCEND_FORGE_ENABLE_SANITIZERS "Enable Address+UBSan on project targets" OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Dependencies.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CWarnings.cmake")

if(ASCEND_FORGE_BUILD_PRIMITIVES OR ASCEND_FORGE_BUILD_APP)
  enable_language(C)
  if(APPLE)
    enable_language(OBJC)
  endif()
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/SdlStatic.cmake")
endif()

# Apply sanitizer flags only to targets created after this point (not FetchContent deps).
if(ASCEND_FORGE_ENABLE_SANITIZERS)
  cppboot_enable_sanitizers()
endif()

# Generate the version API from cmake/version.{h,c}.in (values from VERSION).
set(ASCEND_FORGE_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(ASCEND_FORGE_GENERATED_INCLUDE_DIR "${ASCEND_FORGE_GENERATED_DIR}/include")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in"
  "${ASCEND_FORGE_GENERATED_INCLUDE_DIR}/ascend_forge/version.h"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.c.in"
  "${ASCEND_FORGE_GENERATED_DIR}/version.c"
  @ONLY
)

# The Android Prefab AAR and iOS XCFramework and web/Emscripten package ships the core as a static
# archive, so force the core static there (even with --shared). Android
# additionally needs PIC: the archive folds into libascend_forge.so (src/android/).
set(ASCEND_FORGE_CORE_LIB_TYPE STATIC)
if(ANDROID OR IOS OR EMSCRIPTEN)
  set(ASCEND_FORGE_CORE_LIB_TYPE STATIC)
endif()
if(ANDROID)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()
add_library(${PROJECT_NAME}_lib ${ASCEND_FORGE_CORE_LIB_TYPE})
# Namespaced aliases for consumers (add_subdirectory / FetchContent / find_package).
add_library(${PROJECT_NAME}::lib ALIAS ${PROJECT_NAME}_lib)
add_library(${PROJECT_NAME}::ascend_forge ALIAS ${PROJECT_NAME}_lib)

set_target_properties(${PROJECT_NAME}_lib PROPERTIES
  OUTPUT_NAME ascend_forge
  EXPORT_NAME lib
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  C_STANDARD 17
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
)

target_include_directories(${PROJECT_NAME}_lib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${ASCEND_FORGE_GENERATED_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

ascend_forge_set_c_warnings(${PROJECT_NAME}_lib)

# Preferred third-party deps are linked on the *app* (and optionally tests), not
# the library. Static libraries propagate private deps into install(EXPORT);
# keeping the library free of FetchContent targets makes find_package clean.
# Add target_link_libraries(... PUBLIC/PRIVATE ...) in component CMakeLists when
# library code needs these deps.

add_subdirectory(src)

if(ASCEND_FORGE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(ASCEND_FORGE_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

# Keep a source-root compile_commands.json for clangd / VS Code IntelliSense.
# Only when this project is top-level (do not rewrite a parent project's link).
if(ASCEND_FORGE_IS_TOP_LEVEL AND CMAKE_EXPORT_COMPILE_COMMANDS)
  if(WIN32)
    add_custom_target(cppboot_compile_commands ALL
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${CMAKE_SOURCE_DIR}/compile_commands.json"
      COMMENT "Copying compile_commands.json to source root for clangd"
      VERBATIM
    )
  else()
    add_custom_target(cppboot_compile_commands ALL
      COMMAND ${CMAKE_COMMAND} -E rm -f
        "${CMAKE_SOURCE_DIR}/compile_commands.json"
      COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${CMAKE_SOURCE_DIR}/compile_commands.json"
      COMMENT "Linking compile_commands.json to source root for clangd"
      VERBATIM
    )
  endif()
endif()

include(GNUInstallDirs)
# Desktop install of the C core stays free of the primitive headers: that
# archive does not contain them. The web package merges static SDL into
# libascend_forge.a below, so its install ships the primitive headers and the SDL headers.
if(EMSCRIPTEN AND ASCEND_FORGE_BUILD_PRIMITIVES)
  install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
  )
  install(DIRECTORY "${ASCEND_FORGE_SDL_SOURCE_DIR}/include/SDL3"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
  )
else()
  install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
    PATTERN "arena.h" EXCLUDE
    PATTERN "utf8.h" EXCLUDE
    PATTERN "result.h" EXCLUDE
    PATTERN "io.h" EXCLUDE
    PATTERN "json.h" EXCLUDE
    PATTERN "slice.h" EXCLUDE
    PATTERN "atomic.h" EXCLUDE
    PATTERN "thread.h" EXCLUDE
    PATTERN "time.h" EXCLUDE
    PATTERN "index.h" EXCLUDE
  )
endif()
# Generated version API header (from VERSION + cmake/version.h.in).
install(DIRECTORY ${ASCEND_FORGE_GENERATED_INCLUDE_DIR}/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)
install(TARGETS ${PROJECT_NAME}_lib
  EXPORT ${PROJECT_NAME}Targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# CMake package config so consumers can find_package(ascend-forge).
install(EXPORT ${PROJECT_NAME}Targets
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ascend-forgeConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/ascend-forgeConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/ascend-forgeConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/ascend-forgeConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/ascend-forgeConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# The exported CMake target stays the core archive so it does not name the
# unexported SDL3-static target. On the web, replace the installed file with
# one archive that contains the core, the primitives, and static SDL.
if(EMSCRIPTEN AND ASCEND_FORGE_BUILD_PRIMITIVES)
  install(CODE "
    execute_process(
      COMMAND \"${CMAKE_COMMAND}\" -E env bash
        \"${CMAKE_SOURCE_DIR}/scripts/merge_static_archives.sh\"
        \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/libascend_forge.a\"
        \"${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/libascend_forge.a\"
        \"${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/libascend_forge_primitives.a\"
        \"${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/libSDL3.a\"
      COMMAND_ERROR_IS_FATAL ANY
    )
  ")
endif()

