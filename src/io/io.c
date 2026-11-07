/**
 * @file io.c
 * @brief SDL file and path wrappers.
 */

#include "ascend_forge/io.h"

#include <stddef.h>

static bool borrow_cstr(const char *text, Utf8 *out)
{
    return Utf8_Borrow(text, (Uint64)SDL_strlen(text), out);
}

static bool fail_text(Uint32 code, Utf8 expected, Utf8 actual, Utf8 next, Result *result)
{
    if (!Result_Fail(code, expected, actual, next, result)) {
        return false;
    }
    return false;
}

static bool fail_static(Uint32 code, const char *expected, const char *actual, const char *next,
                        Result *result)
{
    Utf8 expected_text;
    Utf8 actual_text;
    Utf8 next_text;

    if (!borrow_cstr(expected, &expected_text) || !borrow_cstr(actual, &actual_text) ||
        !borrow_cstr(next, &next_text)) {
        return false;
    }
    return fail_text(code, expected_text, actual_text, next_text, result);
}

static bool reject_arguments(Result *result)
{
    return fail_static((Uint32)IoCode_Path, "a caller arena, path, and output", "a null argument",
                       "pass the caller's arena and a real output", result);
}

static bool make_cstring(Arena *arena, Utf8 text, const char **out)
{
    Utf8Builder builder;

    if (!Utf8Builder_Init(&builder, arena)) {
        return false;
    }
    if (!Utf8Builder_Append(&builder, text)) {
        return false;
    }
    return Utf8Builder_CString(&builder, out);
}

static bool copy_text(Arena *arena, const char *text, Utf8 *out)
{
    Utf8 piece;
    Utf8Builder builder;

    if (text == NULL || !borrow_cstr(text, &piece)) {
        return false;
    }
    if (!Utf8Builder_Init(&builder, arena) || !Utf8Builder_Append(&builder, piece)) {
        return false;
    }
    *out = Utf8Builder_View(&builder);
    return true;
}

static bool adopt_sdl_string(Arena *arena, char *owned, Utf8 *out)
{
    const bool copied = owned != NULL && copy_text(arena, owned, out);

    SDL_free(owned);
    return copied;
}

static bool format_u64(Arena *arena, Uint64 value, Utf8 *out)
{
    char stack[32];
    int written = 0;
    Utf8 text;
    Utf8Builder builder;

    written = SDL_snprintf(stack, sizeof(stack), "%llu", (unsigned long long)value);
    if (written <= 0 || (size_t)written >= sizeof(stack)) {
        return false;
    }
    if (!Utf8_Borrow(stack, (Uint64)written, &text)) {
        return false;
    }
    if (!Utf8Builder_Init(&builder, arena) || !Utf8Builder_Append(&builder, text)) {
        return false;
    }
    *out = Utf8Builder_View(&builder);
    return true;
}

static bool is_separator(char mark)
{
    const unsigned char value = (unsigned char)mark;

    return value == (unsigned char)'/' || value == (unsigned char)'\\';
}

static Utf8 separator_text(void)
{
#if defined(SDL_PLATFORM_WINDOWS)
    static char mark = '\\';
#else
    static char mark = '/';
#endif
    Utf8 text;

    text.data = &mark;
    text.size = 1u;
    return text;
}

static bool ends_with_separator(Utf8 text)
{
    if (text.size == 0u || text.data == NULL) {
        return false;
    }
    return is_separator(text.data[text.size - 1u]);
}

static bool starts_with_separator(Utf8 text)
{
    if (text.size == 0u || text.data == NULL) {
        return false;
    }
    return is_separator(text.data[0]);
}

static bool usable_text(Utf8 text)
{
    return text.data != NULL;
}

static bool usable_bytes(Bytes data)
{
    return data.data != NULL;
}

static bool fail_missing(Utf8 path, Result *result)
{
    Utf8 actual;
    Utf8 next;

    if (!borrow_cstr("missing", &actual) || !borrow_cstr("pass a path that exists", &next)) {
        return false;
    }
    return fail_text((Uint32)IoCode_Missing, path, actual, next, result);
}

static bool require_result(const Result *result, const char *name)
{
    if (result != NULL) {
        return true;
    }
    SDL_Log("%s expected a result, received NULL", name);
    return false;
}

static void clear_path_info(SDL_PathInfo *info)
{
    info->type = SDL_PATHTYPE_NONE;
    info->size = 0u;
    info->create_time = 0;
    info->modify_time = 0;
    info->access_time = 0;
}

bool Io_ReadFile(Arena *arena, Utf8 path, Bytes *out, Result *result)
{
    const char *path_text = NULL;
    SDL_IOStream *stream = NULL;
    SDL_PathInfo info;
    Uint8 *block = NULL;
    Bytes loaded;
    size_t want = 0u;
    size_t got = 0u;
    bool closed = false;

    clear_path_info(&info);
    loaded = Bytes_Empty();
    if (!require_result(result, "Io_ReadFile")) {
        return false;
    }
    if (arena == NULL || out == NULL || !usable_text(path)) {
        return reject_arguments(result);
    }
    if (path.size == 0u) {
        return fail_static((Uint32)IoCode_Missing, "a path", "an empty path", "pass a path that exists",
                           result);
    }
    if (!make_cstring(arena, path, &path_text)) {
        return fail_static((Uint32)IoCode_Path, "room in the arena", "the path did not fit",
                           "use a larger arena", result);
    }
    if (!SDL_GetPathInfo(path_text, &info) || info.type == SDL_PATHTYPE_NONE) {
        return fail_missing(path, result);
    }
    if (info.type != SDL_PATHTYPE_FILE) {
        return fail_static((Uint32)IoCode_Path, "a file", "not a file", "pass a file path", result);
    }
    if (info.size > (Uint64)SIZE_MAX) {
        return fail_static((Uint32)IoCode_Path, "a size that fits", "a size past size_t",
                           "read a smaller file", result);
    }
    stream = SDL_IOFromFile(path_text, "rb");
    if (stream == NULL) {
        return fail_missing(path, result);
    }
    if (info.size > 0u && !Arena_Alloc(arena, info.size, 1u, (void **)&block)) {
        SDL_CloseIO(stream);
        return fail_static((Uint32)IoCode_Path, "room in the arena", "the file did not fit",
                           "use a larger arena", result);
    }
    want = (size_t)info.size;
    while (got < want) {
        const size_t n = SDL_ReadIO(stream, block + got, want - got);

        if (n == 0u) {
            break;
        }
        got += n;
    }
    closed = SDL_CloseIO(stream);
    if (got != want) {
        Utf8 expected_count;
        Utf8 actual_count;
        Utf8 next;

        if (!format_u64(arena, (Uint64)want, &expected_count) ||
            !format_u64(arena, (Uint64)got, &actual_count) ||
            !borrow_cstr("read the file again", &next)) {
            return fail_static((Uint32)IoCode_Short, "every requested byte", "a short read",
                               "read the file again", result);
        }
        return fail_text((Uint32)IoCode_Short, expected_count, actual_count, next, result);
    }
    if (!closed) {
        return fail_static((Uint32)IoCode_Path, "a closed file", "a close error", "read the file again",
                           result);
    }
    if (info.size == 0u) {
        loaded = Bytes_Empty();
    } else if (!Bytes_Borrow(block, info.size, &loaded)) {
        return fail_static((Uint32)IoCode_Path, "the file bytes", "a borrow failure", "read the file again",
                           result);
    }
    *out = loaded;
    *result = Result_Ok();
    return true;
}

bool Io_WriteFile(Arena *arena, Utf8 path, Bytes data, Result *result)
{
    const char *path_text = NULL;
    SDL_IOStream *stream = NULL;
    SDL_PathInfo info;
    size_t want = 0u;
    size_t written = 0u;
    bool closed = false;

    clear_path_info(&info);
    if (!require_result(result, "Io_WriteFile")) {
        return false;
    }
    if (arena == NULL || !usable_text(path) || !usable_bytes(data)) {
        return reject_arguments(result);
    }
    if (path.size == 0u) {
        return fail_static((Uint32)IoCode_Path, "a path", "an empty path", "pass a path the job can write",
                           result);
    }
    if (data.size > (Uint64)SIZE_MAX) {
        return fail_static((Uint32)IoCode_Path, "a size that fits", "a size past size_t",
                           "write a smaller span", result);
    }
    if (!make_cstring(arena, path, &path_text)) {
        return fail_static((Uint32)IoCode_Path, "room in the arena", "the path did not fit",
                           "use a larger arena", result);
    }
    if (SDL_GetPathInfo(path_text, &info) && info.type == SDL_PATHTYPE_DIRECTORY) {
        return fail_static((Uint32)IoCode_Path, "a file", "a directory", "pass a file path", result);
    }
    stream = SDL_IOFromFile(path_text, "wb");
    if (stream == NULL) {
        return fail_static((Uint32)IoCode_Missing, "a writable file", "missing",
                           "pass a path the job can write", result);
    }
    want = (size_t)data.size;
    written = SDL_WriteIO(stream, data.data, want);
    closed = SDL_CloseIO(stream);
    if (written != want) {
        Utf8 expected_count;
        Utf8 actual_count;
        Utf8 next;

        if (!format_u64(arena, (Uint64)want, &expected_count) ||
            !format_u64(arena, (Uint64)written, &actual_count) ||
            !borrow_cstr("write the file again", &next)) {
            return fail_static((Uint32)IoCode_Short, "every byte", "a short write", "write the file again",
                               result);
        }
        return fail_text((Uint32)IoCode_Short, expected_count, actual_count, next, result);
    }
    if (!closed) {
        return fail_static((Uint32)IoCode_Path, "a closed file", "a close error", "write the file again",
                           result);
    }
    *result = Result_Ok();
    return true;
}

bool Io_JoinPath(Arena *arena, Utf8 left, Utf8 right, Utf8 *out, Result *result)
{
    Utf8Builder builder;
    Utf8 joined;
    bool need_separator = false;
    Uint64 total = 0u;

    joined = Utf8_Empty();
    if (!require_result(result, "Io_JoinPath")) {
        return false;
    }
    if (arena == NULL || out == NULL || !usable_text(left) || !usable_text(right)) {
        return reject_arguments(result);
    }
    need_separator = left.size > 0u && right.size > 0u && !ends_with_separator(left) &&
                     !starts_with_separator(right);
    if (right.size > ~(Uint64)0 - left.size) {
        return fail_static((Uint32)IoCode_Path, "a path that fits", "a path that overflows",
                           "join shorter paths", result);
    }
    total = left.size + right.size;
    if (need_separator && total == ~(Uint64)0) {
        return fail_static((Uint32)IoCode_Path, "a path that fits", "a path that overflows",
                           "join shorter paths", result);
    }
    if (!Utf8Builder_Init(&builder, arena)) {
        return reject_arguments(result);
    }
    if ((left.size > 0u && !Utf8Builder_Append(&builder, left)) ||
        (need_separator && !Utf8Builder_Append(&builder, separator_text())) ||
        (right.size > 0u && !Utf8Builder_Append(&builder, right))) {
        return fail_static((Uint32)IoCode_Path, "room in the arena", "the path did not fit",
                           "use a larger arena", result);
    }
    joined = Utf8Builder_View(&builder);
    *out = joined;
    *result = Result_Ok();
    return true;
}

bool Io_BasePath(Arena *arena, Utf8 *out, Result *result)
{
    Utf8 copied;

    copied = Utf8_Empty();
    if (!require_result(result, "Io_BasePath")) {
        return false;
    }
    if (arena == NULL || out == NULL) {
        return reject_arguments(result);
    }
    if (!copy_text(arena, SDL_GetBasePath(), &copied)) {
        return fail_static((Uint32)IoCode_Path, "a base path", "unavailable",
                           "run where the platform provides a base path", result);
    }
    *out = copied;
    *result = Result_Ok();
    return true;
}

bool Io_PrefPath(Arena *arena, Utf8 org, Utf8 app, Utf8 *out, Result *result)
{
    const char *org_text = NULL;
    const char *app_text = NULL;
    Utf8 copied;

    copied = Utf8_Empty();
    if (!require_result(result, "Io_PrefPath")) {
        return false;
    }
    if (arena == NULL || out == NULL || !usable_text(org) || !usable_text(app)) {
        return reject_arguments(result);
    }
    if (app.size == 0u) {
        return fail_static((Uint32)IoCode_Path, "an app name", "an empty app name", "pass a stable app name",
                           result);
    }
    if (!make_cstring(arena, org, &org_text) || !make_cstring(arena, app, &app_text)) {
        return fail_static((Uint32)IoCode_Path, "room in the arena", "the path did not fit",
                           "use a larger arena", result);
    }
    if (!adopt_sdl_string(arena, SDL_GetPrefPath(org_text, app_text), &copied)) {
        return fail_static((Uint32)IoCode_Path, "a preference path", "unavailable",
                           "pass an org and app the platform can store", result);
    }
    *out = copied;
    *result = Result_Ok();
    return true;
}

bool Io_PathInfo(Arena *arena, Utf8 path, IoPathInfo *out, Result *result)
{
    const char *path_text = NULL;
    SDL_PathInfo info;
    IoPathInfo reported;

    clear_path_info(&info);
    reported.exists = false;
    reported.directory = false;
    reported.size = 0u;
    if (!require_result(result, "Io_PathInfo")) {
        return false;
    }
    if (arena == NULL || out == NULL || !usable_text(path)) {
        return reject_arguments(result);
    }
    if (path.size == 0u) {
        return fail_static((Uint32)IoCode_Path, "a path", "an empty path", "pass a path", result);
    }
    if (!make_cstring(arena, path, &path_text)) {
        return fail_static((Uint32)IoCode_Path, "room in the arena", "the path did not fit",
                           "use a larger arena", result);
    }
    if (!SDL_GetPathInfo(path_text, &info) || info.type == SDL_PATHTYPE_NONE) {
        *out = reported;
        *result = Result_Ok();
        return true;
    }
    reported.exists = true;
    reported.directory = info.type == SDL_PATHTYPE_DIRECTORY;
    reported.size = info.size;
    *out = reported;
    *result = Result_Ok();
    return true;
}
