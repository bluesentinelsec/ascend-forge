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
