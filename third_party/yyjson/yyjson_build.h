/**
 * @file yyjson_build.h
 * @brief Ascend Forge build of yyjson 0.13.0.
 *
 * Include this before yyjson.h. The parser then builds without libc
 * malloc, file APIs, or the JSON Pointer helpers. Memory and string calls
 * go through yyjson_sdl.h. Json_ supplies the arena allocator per call.
 */

#ifndef ASCEND_FORGE_YYJSON_BUILD_H
#define ASCEND_FORGE_YYJSON_BUILD_H

#define YYJSON_FREESTANDING 1
#define YYJSON_DISABLE_FILE 1
#define YYJSON_DISABLE_UTILS 1
#define YYJSON_DISABLE_INCR_READER 1
#define YYJSON_DISABLE_NON_STANDARD 1
#define YYJSON_FREESTANDING_HEADER "yyjson_sdl.h"

#ifndef yyjson_api
#if defined(__GNUC__) || defined(__clang__)
#define yyjson_api __attribute__((visibility("hidden")))
#else
#define yyjson_api
#endif
#endif

#endif /* ASCEND_FORGE_YYJSON_BUILD_H */
