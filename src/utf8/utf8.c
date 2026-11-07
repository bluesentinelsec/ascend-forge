/**
 * @file utf8.c
 * @brief Borrowed byte spans and a one-owner UTF-8 builder.
 */

#include "ascend_forge/utf8.h"

#include <stddef.h>
#include <stdint.h>

/** Address used when a span is empty and the caller did not borrow one. */
static char g_utf8_empty = '\0';

static Bytes bytes_empty(void)
{
    Bytes bytes;

    bytes.data = (const Uint8 *)&g_utf8_empty;
    bytes.size = 0u;
    return bytes;
}

static Utf8 utf8_empty(void)
{
    Utf8 text;

    text.data = &g_utf8_empty;
    text.size = 0u;
    return text;
}

static bool size_fits(Uint64 size)
{
    return size <= (Uint64)SIZE_MAX;
}

static size_t size_cast(Uint64 size)
{
    return (size_t)size;
}

Bytes Bytes_Empty(void)
{
    return bytes_empty();
}

bool Bytes_Borrow(const Uint8 *data, Uint64 size, Bytes *out)
{
    if (out == NULL) {
        SDL_Log("Bytes_Borrow expected an out parameter, received NULL");
        return false;
    }
    *out = bytes_empty();
    if (data == NULL) {
        SDL_Log("Bytes_Borrow expected bytes, received NULL");
        return false;
    }
    if (!size_fits(size)) {
        SDL_Log("Bytes_Borrow expected a size that fits in size_t, received %llu",
                (unsigned long long)size);
        return false;
    }
    out->data = data;
    out->size = size;
    return true;
}

int Bytes_Compare(Bytes left, Bytes right)
{
    size_t prefix = 0u;
    int order = 0;

    if (left.data == NULL || right.data == NULL || !size_fits(left.size) || !size_fits(right.size)) {
        SDL_Log("Bytes_Compare expected spans with real pointers");
        return left.size < right.size ? -1 : (left.size > right.size ? 1 : 0);
    }
    if (left.size < right.size) {
        prefix = size_cast(left.size);
    } else {
        prefix = size_cast(right.size);
    }
    if (prefix > 0u) {
        order = SDL_memcmp(left.data, right.data, prefix);
    }
    if (order != 0) {
        return order;
    }
    if (left.size < right.size) {
        return -1;
    }
    if (left.size > right.size) {
        return 1;
    }
    return 0;
}

bool Bytes_Equal(Bytes left, Bytes right)
{
    return Bytes_Compare(left, right) == 0;
}

bool Bytes_HasPrefix(Bytes text, Bytes prefix)
{
    size_t prefix_size = 0u;

    if (text.data == NULL || prefix.data == NULL || !size_fits(text.size) || !size_fits(prefix.size)) {
        SDL_Log("Bytes_HasPrefix expected spans with real pointers");
        return false;
    }
    if (prefix.size > text.size) {
        return false;
    }
    if (prefix.size == 0u) {
        return true;
    }
    prefix_size = size_cast(prefix.size);
    return SDL_memcmp(text.data, prefix.data, prefix_size) == 0;
}

bool Bytes_HasSuffix(Bytes text, Bytes suffix)
{
    size_t suffix_size = 0u;

    if (text.data == NULL || suffix.data == NULL || !size_fits(text.size) || !size_fits(suffix.size)) {
        SDL_Log("Bytes_HasSuffix expected spans with real pointers");
        return false;
    }
    if (suffix.size > text.size) {
        return false;
    }
    if (suffix.size == 0u) {
        return true;
    }
    suffix_size = size_cast(suffix.size);
    return SDL_memcmp(text.data + size_cast(text.size - suffix.size), suffix.data, suffix_size) == 0;
}

Utf8 Utf8_Empty(void)
{
    return utf8_empty();
}

bool Utf8_Borrow(const char *data, Uint64 size, Utf8 *out)
{
    if (out == NULL) {
        SDL_Log("Utf8_Borrow expected an out parameter, received NULL");
        return false;
    }
    *out = utf8_empty();
    if (data == NULL) {
        SDL_Log("Utf8_Borrow expected text, received NULL");
        return false;
    }
    if (!size_fits(size)) {
        SDL_Log("Utf8_Borrow expected a size that fits in size_t, received %llu",
                (unsigned long long)size);
        return false;
    }
    out->data = data;
    out->size = size;
    return true;
}

static bool utf8_usable(Utf8 text)
{
    return text.data != NULL && size_fits(text.size);
}

int Utf8_Compare(Utf8 left, Utf8 right)
{
    size_t prefix = 0u;
    int order = 0;

    if (!utf8_usable(left) || !utf8_usable(right)) {
        SDL_Log("Utf8_Compare expected text with real pointers");
        return left.size < right.size ? -1 : (left.size > right.size ? 1 : 0);
    }
    if (left.size < right.size) {
        prefix = size_cast(left.size);
    } else {
        prefix = size_cast(right.size);
    }
    if (prefix > 0u) {
        order = SDL_strncmp(left.data, right.data, prefix);
    }
    if (order != 0) {
        return order;
    }
    if (left.size < right.size) {
        return -1;
    }
    if (left.size > right.size) {
        return 1;
    }
    return 0;
}

bool Utf8_Equal(Utf8 left, Utf8 right)
{
    return Utf8_Compare(left, right) == 0;
}

bool Utf8_HasPrefix(Utf8 text, Utf8 prefix)
{
    size_t prefix_size = 0u;

    if (!utf8_usable(text) || !utf8_usable(prefix)) {
        SDL_Log("Utf8_HasPrefix expected text with real pointers");
        return false;
    }
    if (prefix.size > text.size) {
        return false;
    }
    if (prefix.size == 0u) {
        return true;
    }
    prefix_size = size_cast(prefix.size);
    return SDL_strncmp(text.data, prefix.data, prefix_size) == 0;
}

bool Utf8_HasSuffix(Utf8 text, Utf8 suffix)
{
    size_t suffix_size = 0u;
    const char *tail = NULL;

    if (!utf8_usable(text) || !utf8_usable(suffix)) {
        SDL_Log("Utf8_HasSuffix expected text with real pointers");
        return false;
    }
    if (suffix.size > text.size) {
        return false;
    }
    if (suffix.size == 0u) {
        return true;
    }
    suffix_size = size_cast(suffix.size);
    tail = text.data + size_cast(text.size - suffix.size);
    return SDL_strncmp(tail, suffix.data, suffix_size) == 0;
}

bool Utf8_Next(Utf8 *text, Uint32 *codepoint)
{
    const char *cursor = NULL;
    const char *before = NULL;
    size_t remaining = 0u;
    Uint32 value = 0u;

    if (text == NULL || codepoint == NULL) {
        SDL_Log("Utf8_Next expected text and a code point, received NULL");
        return false;
    }
    if (!utf8_usable(*text) || text->size == 0u) {
        return false;
    }
    cursor = text->data;
    remaining = size_cast(text->size);
    if (cursor[0] == '\0') {
        *codepoint = 0u;
        text->data = cursor + 1;
        text->size -= 1u;
        return true;
    }
    before = cursor;
    value = SDL_StepUTF8(&cursor, &remaining);
    if (cursor == before) {
        return false;
    }
    *codepoint = value;
    text->data = cursor;
    text->size = (Uint64)remaining;
    return true;
}

bool Utf8_IsValid(Utf8 text)
{
    while (text.size > 0u) {
        const char *before = text.data;
        const Uint64 before_size = text.size;
        Uint32 codepoint = 0u;
        Uint64 consumed = 0u;

        if (!Utf8_Next(&text, &codepoint)) {
            return false;
        }
        if (codepoint != SDL_INVALID_UNICODE_CODEPOINT) {
            continue;
        }
        consumed = before_size - text.size;
        if (consumed == 3u &&
            (Uint8)before[0] == 0xEFu &&
            (Uint8)before[1] == 0xBFu &&
            (Uint8)before[2] == 0xBDu) {
            continue;
        }
        return false;
    }
    return true;
}

Uint64 Utf8_CodePointCount(Utf8 text)
{
    Uint64 count = 0u;
    Uint32 codepoint = 0u;

    while (Utf8_Next(&text, &codepoint)) {
        count += 1u;
    }
    return count;
}

bool Utf8Builder_Init(Utf8Builder *builder, Arena *arena)
{
    if (builder == NULL) {
        SDL_Log("Utf8Builder_Init expected a builder, received NULL");
        return false;
    }
    builder->arena = NULL;
    builder->data = &g_utf8_empty;
    builder->size = 0u;
    builder->capacity = 0u;
    if (arena == NULL) {
        SDL_Log("Utf8Builder_Init expected an arena, received NULL");
        return false;
    }
    builder->arena = arena;
    return true;
}

static bool builder_reserve(Utf8Builder *builder, Uint64 extra)
{
    Uint64 needed = 0u;
    Uint64 grown = 0u;
    char *block = NULL;

    if (extra > (~(Uint64)0) - builder->size) {
        SDL_Log("Utf8Builder_Append expected a length that fits, received %llu plus %llu",
                (unsigned long long)builder->size,
                (unsigned long long)extra);
        return false;
    }
    needed = builder->size + extra;
    if (needed <= builder->capacity) {
        return true;
    }
    if (builder->capacity <= (~(Uint64)0) / 2u) {
        grown = builder->capacity * 2u;
    } else {
        grown = ~(Uint64)0;
    }
    if (grown < needed) {
        grown = needed;
    }
    if (!Arena_Alloc(builder->arena, grown, 1u, (void **)&block)) {
        return false;
    }
    if (builder->size > 0u) {
        SDL_memcpy(block, builder->data, size_cast(builder->size));
    }
    builder->data = block;
    builder->capacity = grown;
    return true;
}

bool Utf8Builder_Append(Utf8Builder *builder, Utf8 text)
{
    if (builder == NULL || builder->arena == NULL || builder->data == NULL) {
        SDL_Log("Utf8Builder_Append expected a live builder");
        return false;
    }
    if (!utf8_usable(text)) {
        SDL_Log("Utf8Builder_Append expected text with a real pointer");
        return false;
    }
    if (text.size == 0u) {
        return true;
    }
    if (!builder_reserve(builder, text.size)) {
        return false;
    }
    SDL_memcpy(builder->data + size_cast(builder->size), text.data, size_cast(text.size));
    builder->size += text.size;
    return true;
}

Utf8 Utf8Builder_View(const Utf8Builder *builder)
{
    Utf8 text = utf8_empty();

    if (builder == NULL || builder->data == NULL) {
        return text;
    }
    text.data = builder->data;
    text.size = builder->size;
    return text;
}

bool Utf8Builder_CString(const Utf8Builder *builder, const char **out)
{
    char *terminated = NULL;

    if (out == NULL) {
        SDL_Log("Utf8Builder_CString expected an out parameter, received NULL");
        return false;
    }
    *out = &g_utf8_empty;
    if (builder == NULL || builder->arena == NULL || builder->data == NULL) {
        SDL_Log("Utf8Builder_CString expected a live builder");
        return false;
    }
    if (builder->size == 0u) {
        return true;
    }
    if (builder->size > (Uint64)SIZE_MAX - 1u) {
        SDL_Log("Utf8Builder_CString expected a length that fits with a NUL, received %llu",
                (unsigned long long)builder->size);
        return false;
    }
    if (!Arena_Alloc(builder->arena, builder->size + 1u, 1u, (void **)&terminated)) {
        return false;
    }
    SDL_memcpy(terminated, builder->data, size_cast(builder->size));
    terminated[size_cast(builder->size)] = '\0';
    *out = terminated;
    return true;
}
