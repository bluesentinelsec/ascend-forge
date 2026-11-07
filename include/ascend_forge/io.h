/**
 * @file io.h
 * @brief File and path calls on SDL.
 *
 * Read files into the caller's arena. Write a span to a path that one job
 * owns. Join paths as arena text. Base and preference directories come from
 * SDL. A missing file or a short transfer returns a result and does not exit.
 * Out of memory stays fatal inside the arena.
 *
 * A failure leaves the output span or path info untouched. A short read still
 * consumes arena space for the attempt. A short write may leave a partial
 * file. Two jobs do not write the same path.
 */

#ifndef ASCEND_FORGE_IO_H
#define ASCEND_FORGE_IO_H

#include "ascend_forge/result.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Failure codes stored in a Result from Io_. Zero remains success. */
typedef enum IoCode {
    IoCode_Missing = 1,
    IoCode_Short = 2,
    IoCode_Path = 3
} IoCode;

/** What SDL_GetPathInfo reported. exists is false when the path is absent. */
typedef struct IoPathInfo {
    bool exists;
    bool directory;
    Uint64 size;
} IoPathInfo;

/**
 * @brief Read path into out.
 *
 * @param arena Receives the file bytes and any counted error text.
 * @param path File path. It does not need a NUL terminator.
 * @param out Receives the bytes. On failure this is left untouched. An empty
 *        file is a real pointer and a length of zero.
 * @param result Receives success or the failure. On rejection of a NULL
 *        result this is not written.
 * @return false when the file is missing, the read is short, or an argument
 *         is unusable.
 */
bool Io_ReadFile(Arena *arena, Utf8 path, Bytes *out, Result *result);

/**
 * @brief Write data to path, replacing any file already there.
 *
 * The job that calls this owns path for the write.
 *
 * @param arena Holds the NUL path and any counted error text.
 * @param path Destination path. It does not need a NUL terminator.
 * @param data Bytes to write. An empty span creates an empty file.
 * @param result Receives success or the failure.
 * @return false when the write is short, the path is a directory, or an
 *         argument is unusable.
 */
bool Io_WriteFile(Arena *arena, Utf8 path, Bytes data, Result *result);

/**
 * @brief Join left and right into one path in the arena.
 *
 * A separator is inserted only when both sides are non-empty and neither
 * side already has one at the boundary. An empty side copies the other side.
 *
 * @param out Receives the joined path. On failure this is left untouched.
 * @return false when the join does not fit or an argument is unusable.
 */
bool Io_JoinPath(Arena *arena, Utf8 left, Utf8 right, Utf8 *out, Result *result);

/**
 * @brief Copy SDL_GetBasePath into the arena.
 *
 * The copy ends with the platform separator when SDL provides one.
 *
 * @param out Receives the path. On failure this is left untouched.
 */
bool Io_BasePath(Arena *arena, Utf8 *out, Result *result);

/**
 * @brief Copy SDL_GetPrefPath into the arena.
 *
 * This is the directory a job may write. The copy ends with the platform
 * separator when SDL provides one. app must be non-empty. org may be empty.
 *
 * @param out Receives the directory. On failure this is left untouched.
 */
bool Io_PrefPath(Arena *arena, Utf8 org, Utf8 app, Utf8 *out, Result *result);

/**
 * @brief Report SDL_GetPathInfo for path.
 *
 * A missing path succeeds with exists set to false.
 *
 * @param out Receives the info. On failure this is left untouched.
 */
bool Io_PathInfo(Arena *arena, Utf8 path, IoPathInfo *out, Result *result);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_IO_H */
