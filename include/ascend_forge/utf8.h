/**
 * @file utf8.h
 * @brief Borrowed byte spans and UTF-8 text.
 *
 * A span is a pointer and a length. It does not own the bytes. The empty
 * span is a real pointer and a length of zero. An immutable span may be
 * read by many jobs. A builder has one owner and appends into that owner's
 * arena.
 */

#ifndef ASCEND_FORGE_UTF8_H
#define ASCEND_FORGE_UTF8_H

#include "ascend_forge/arena.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Borrowed bytes. data is never NULL. */
typedef struct Bytes {
    const Uint8 *data;
    Uint64 size;
} Bytes;

/** Borrowed UTF-8 text, counted in bytes. data is never NULL. */
typedef struct Utf8 {
    const char *data;
    Uint64 size;
} Utf8;

/**
 * @brief One-owner UTF-8 builder.
 *
 * The text lives in arena. Jobs may read a Utf8 borrowed from the builder.
 * They do not share the builder. The stored text has no NUL terminator.
 */
typedef struct Utf8Builder {
    Arena *arena;
    char *data;
    Uint64 size;
    Uint64 capacity;
} Utf8Builder;

/** @brief Empty byte span. The pointer is real and the length is zero. */
Bytes Bytes_Empty(void);

/**
 * @brief Borrow size bytes at data.
 *
 * @param out Receives the span. On failure this is an empty span.
 * @return false when data is NULL, size does not fit in size_t, or out is
 *         NULL. A size of zero succeeds and keeps data.
 */
bool Bytes_Borrow(const Uint8 *data, Uint64 size, Bytes *out);

/** @brief True when the sizes match and SDL_memcmp matches. */
bool Bytes_Equal(Bytes left, Bytes right);

/**
 * @brief Order left against right with SDL_memcmp, then by size.
 *
 * @return Negative when left is less, positive when left is greater, and
 *         zero when the spans match.
 */
int Bytes_Compare(Bytes left, Bytes right);

/** @brief True when prefix is empty or a leading SDL_memcmp match. */
bool Bytes_HasPrefix(Bytes text, Bytes prefix);

/** @brief True when suffix is empty or a trailing SDL_memcmp match. */
bool Bytes_HasSuffix(Bytes text, Bytes suffix);

/** @brief Empty UTF-8 text. The pointer is real and the length is zero. */
Utf8 Utf8_Empty(void);

/**
 * @brief Borrow size bytes of text at data.
 *
 * The bytes do not need a NUL terminator. A size of zero succeeds and keeps
 * data.
 *
 * @param out Receives the text. On failure this is empty.
 * @return false when data is NULL, size does not fit in size_t, or out is
 *         NULL.
 */
bool Utf8_Borrow(const char *data, Uint64 size, Utf8 *out);

/** @brief True when SDL_strncmp reports a match over the shared length. */
bool Utf8_Equal(Utf8 left, Utf8 right);

/**
 * @brief Order left against right with SDL_strncmp, then by size.
 *
 * SDL_strncmp stops at a zero byte. A zero byte inside the text ends the
 * comparison early.
 */
int Utf8_Compare(Utf8 left, Utf8 right);

/** @brief True when prefix is empty or SDL_strncmp matches at the start. */
bool Utf8_HasPrefix(Utf8 text, Utf8 prefix);

/** @brief True when suffix is empty or SDL_strncmp matches at the end. */
bool Utf8_HasSuffix(Utf8 text, Utf8 suffix);

/**
 * @brief True when every byte sequence is well-formed UTF-8.
 *
 * The walk uses SDL_StepUTF8. U+FFFD encoded as EF BF BD is valid. Any
 * other sequence that SDL reports as the replacement character is not.
 */
bool Utf8_IsValid(Utf8 text);

/**
 * @brief Read one code point and advance text past the bytes it consumed.
 *
 * A leading zero byte is U+0000 and consumes one byte. SDL_StepUTF8 does
 * not advance on U+0000, so that byte is handled before the SDL walk.
 *
 * @param text Text to read and advance. On failure text is unchanged.
 * @param codepoint Receives the code point. Unchanged when text is empty.
 * @return false when text is empty or an argument is unusable.
 */
bool Utf8_Next(Utf8 *text, Uint32 *codepoint);

/** @brief Code points in text, using the same walk as Utf8_Next. */
Uint64 Utf8_CodePointCount(Utf8 text);

/**
 * @brief Point builder at arena with empty text.
 *
 * @return false when builder or arena is NULL. A failed builder has empty
 *         text and rejects appends.
 */
bool Utf8Builder_Init(Utf8Builder *builder, Arena *arena);

/**
 * @brief Append text into the builder's arena.
 *
 * A new block is allocated when the spare capacity is too small. The old
 * text is copied with SDL_memcpy. Failure leaves the builder unchanged.
 * An empty append succeeds and consumes nothing.
 */
bool Utf8Builder_Append(Utf8Builder *builder, Utf8 text);

/** @brief Borrow the builder's text. The result has no NUL terminator. */
Utf8 Utf8Builder_View(const Utf8Builder *builder);

/**
 * @brief Copy the text into the arena and write a NUL after it.
 *
 * This is the SDL and C boundary. The builder's own text stays unterminated.
 *
 * @param out Receives the terminated copy. On failure, or when the text is
 *        empty, this is a real empty string.
 * @return false when the arena cannot hold the copy or an argument is
 *         unusable.
 */
bool Utf8Builder_CString(const Utf8Builder *builder, const char **out);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_UTF8_H */
