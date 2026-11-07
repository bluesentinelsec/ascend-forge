/**
 * @file yyjson_sdl.h
 * @brief SDL replacements for the libc calls yyjson would otherwise use.
 *
 * SDL maps SDL_memcpy, SDL_memmove, and SDL_memset back to the compiler
 * builtins. Those redirects are removed here so this file calls the SDL
 * functions. SDL_memcmp and SDL_strlen are already functions.
 */

#ifndef ASCEND_FORGE_YYJSON_SDL_H
#define ASCEND_FORGE_YYJSON_SDL_H

#include <SDL3/SDL.h>

#ifdef SDL_memcpy
#undef SDL_memcpy
#endif
#ifdef SDL_memmove
#undef SDL_memmove
#endif
#ifdef SDL_memset
#undef SDL_memset
#endif

#ifdef memcpy
#undef memcpy
#endif
#ifdef memmove
#undef memmove
#endif
#ifdef memset
#undef memset
#endif
#ifdef memcmp
#undef memcmp
#endif
#ifdef strlen
#undef strlen
#endif

#define memcpy SDL_memcpy
#define memmove SDL_memmove
#define memset SDL_memset
#define memcmp SDL_memcmp
#define strlen SDL_strlen

#endif /* ASCEND_FORGE_YYJSON_SDL_H */
