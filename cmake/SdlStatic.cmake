# SDL3 is the platform layer. Build the static library only. Callers link
# SDL3::SDL3-static and do not load a system SDL3 dynamic library.
include(FetchContent)

set(SDL3_TAG release-3.4.20)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_TESTS OFF CACHE BOOL "" FORCE)
# Linux configure stops when X11 and Wayland headers are both missing.
# This flag only skips that check. X11 and Wayland are still built when
# their headers are present. Android, iOS, and Emscripten do not use it.
set(SDL_UNIX_CONSOLE_BUILD ON CACHE BOOL "" FORCE)
# This host has its own Android activity. SDL's jar is the SDLActivity
# glue. The native static library still builds the Android backends.
set(SDL_ANDROID_JAR OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  SDL
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG        ${SDL3_TAG}
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SDL)

if(NOT TARGET SDL3::SDL3-static)
  message(FATAL_ERROR "SDL3 static target SDL3::SDL3-static was not created")
endif()
cppboot_mark_system_includes(SDL3-static)

set(ASCEND_FORGE_SDL_SOURCE_DIR "${sdl_SOURCE_DIR}" CACHE INTERNAL
  "Fetched SDL3 source tree"
)
if(NOT IS_DIRECTORY "${ASCEND_FORGE_SDL_SOURCE_DIR}/include/SDL3")
  message(FATAL_ERROR "Fetched SDL3 is missing include/SDL3")
endif()

if(ANDROID)
  # Each ABI configures in parallel and has its own FetchContent tree.
  # The public headers match; keep one marker per ABI so the writes do not race.
  file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/android/ascend_forge/build")
  file(WRITE
    "${CMAKE_SOURCE_DIR}/android/ascend_forge/build/sdl-public-include-${ANDROID_ABI}.txt"
    "${ASCEND_FORGE_SDL_SOURCE_DIR}/include\n"
  )
endif()
