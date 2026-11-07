/**
 * @file json.c
 * @brief JSON values on an arena, parsed by the vendored yyjson reader.
 *
 * yyjson stays inside this library. The arena allocator copies on growth
 * and does not free until the arena dies. A failed parse leaves the caller's
 * output untouched.
 */

#include "ascend_forge/json.h"

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-const-variable"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wshadow"
#endif
#include "yyjson_build.h"
#include "yyjson.h"
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <stdint.h>

static bool size_fits(Uint64 size)
{
    return size <= (Uint64)SIZE_MAX;
}

static size_t size_cast(Uint64 size)
{
    return (size_t)size;
}

static const yyjson_val *json_node(Json value)
{
    return (const yyjson_val *)value.node;
}

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

static bool require_result(const Result *result, const char *name)
{
    if (result != NULL) {
        return true;
    }
    SDL_Log("%s expected a result, received NULL", name);
    return false;
}

static bool reject_parse(Result *result)
{
    return fail_static((Uint32)JsonCode_Invalid, "a caller arena, JSON text, and output",
                       "a null argument", "pass the caller's arena and a real output", result);
}

static bool reject_stringify(Result *result)
{
    return fail_static((Uint32)JsonCode_Invalid, "a JSON value and a builder", "a null argument",
                       "pass a parsed value and the caller's builder", result);
}

static bool fail_capacity(Result *result)
{
    return fail_static((Uint32)JsonCode_Invalid, "one JSON value",
                       "the arena could not hold the JSON value", "give the arena more room", result);
}

static bool copy_message(Arena *arena, const char *message, size_t pos, Utf8 *out)
{
    char stack[192];
    int written = 0;
    Utf8 text;
    Utf8Builder builder;
    const char *detail = message != NULL ? message : "invalid JSON";

    written = SDL_snprintf(stack, sizeof(stack), "%s at byte %llu", detail, (unsigned long long)pos);
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

static bool fail_parse(Arena *arena, const yyjson_read_err *err, Result *result)
{
    Utf8 expected;
    Utf8 actual;
    Utf8 next;

    if (err->code == YYJSON_READ_ERROR_MEMORY_ALLOCATION) {
        return fail_capacity(result);
    }
    if (!borrow_cstr("one JSON value", &expected) ||
        !borrow_cstr("pass one JSON value and no trailing bytes", &next)) {
        return false;
    }
    if (!copy_message(arena, err->msg, err->pos, &actual)) {
        return fail_static((Uint32)JsonCode_Invalid, "one JSON value", "invalid JSON",
                           "pass one JSON value and no trailing bytes", result);
    }
    return fail_text((Uint32)JsonCode_Invalid, expected, actual, next, result);
}

static bool fail_write(const yyjson_write_err *err, Result *result)
{
    if (err->code == YYJSON_WRITE_ERROR_MEMORY_ALLOCATION) {
        return fail_capacity(result);
    }
    return fail_static((Uint32)JsonCode_Invalid, "one JSON value",
                       err->msg != NULL ? err->msg : "invalid JSON",
                       "pass a value from Json_Parse", result);
}

static void *json_malloc(void *ctx, size_t size)
{
    Arena *arena = (Arena *)ctx;
    void *block = NULL;
    Uint64 bytes = (Uint64)size;

    if (bytes == 0u) {
        bytes = 1u;
    }
    if (!Arena_Alloc(arena, bytes, 16u, &block)) {
        return NULL;
    }
    return block;
}

static void *json_realloc(void *ctx, void *ptr, size_t old_size, size_t size)
{
    void *block = NULL;
    size_t copy = old_size < size ? old_size : size;

    block = json_malloc(ctx, size);
    if (block == NULL || ptr == NULL || copy == 0u) {
        return block;
    }
    SDL_memcpy(block, ptr, copy);
    return block;
}

static void json_free(void *ctx, void *ptr)
{
    (void)ctx;
    (void)ptr;
}

static void json_allocator(Arena *arena, yyjson_alc *out)
{
    out->malloc = json_malloc;
    out->realloc = json_realloc;
    out->free = json_free;
    out->ctx = arena;
}

static char *json_source(Utf8 text)
{
    /* NUMBER_AS_RAW does not set YYJSON_READ_INSITU, so the reader does not write this buffer. */
    return (char *)text.data;
}

static bool usable_text(Utf8 text)
{
    return text.data != NULL && size_fits(text.size);
}

static Json json_from(const yyjson_val *node)
{
    Json value;

    value.node = node;
    return value;
}

bool Json_Parse(Arena *arena, Utf8 text, Json *out, Result *result)
{
    yyjson_alc alc;
    yyjson_read_err err;
    yyjson_doc *doc = NULL;
    yyjson_val *root = NULL;

    err.code = YYJSON_READ_SUCCESS;
    err.msg = NULL;
    err.pos = 0u;
    if (!require_result(result, "Json_Parse")) {
        return false;
    }
    if (arena == NULL || out == NULL || !usable_text(text)) {
        return reject_parse(result);
    }

    json_allocator(arena, &alc);
    doc = yyjson_read_opts(json_source(text), size_cast(text.size), YYJSON_READ_NUMBER_AS_RAW, &alc, &err);
    if (doc == NULL) {
        return fail_parse(arena, &err, result);
    }
    root = yyjson_doc_get_root(doc);
    if (root == NULL) {
        return fail_static((Uint32)JsonCode_Invalid, "one JSON value", "invalid JSON",
                           "pass one JSON value and no trailing bytes", result);
    }
    *out = json_from(root);
    *result = Result_Ok();
    return true;
}

bool Json_Stringify(Json value, Utf8Builder *out, Result *result)
{
    yyjson_alc alc;
    yyjson_write_err err;
    const yyjson_val *node = json_node(value);
    char *text = NULL;
    size_t length = 0u;
    Utf8 piece;

    err.code = YYJSON_WRITE_SUCCESS;
    err.msg = NULL;
    if (!require_result(result, "Json_Stringify")) {
        return false;
    }
    if (out == NULL || out->arena == NULL || node == NULL) {
        return reject_stringify(result);
    }

    json_allocator(out->arena, &alc);
    text = yyjson_val_write_opts(node, 0, &alc, &length, &err);
    if (text == NULL) {
        return fail_write(&err, result);
    }
    if (!Utf8_Borrow(text, (Uint64)length, &piece) || !Utf8Builder_Append(out, piece)) {
        return fail_capacity(result);
    }
    *result = Result_Ok();
    return true;
}

bool Json_Kind(Json value, JsonKind *out)
{
    const yyjson_val *node = json_node(value);

    if (out == NULL || node == NULL) {
        SDL_Log("Json_Kind expected a JSON value and output, received NULL");
        return false;
    }
    if (yyjson_is_null(node)) {
        *out = JsonKind_Null;
        return true;
    }
    if (yyjson_is_bool(node)) {
        *out = JsonKind_Bool;
        return true;
    }
    if (yyjson_is_raw(node) || yyjson_is_num(node)) {
        *out = JsonKind_Number;
        return true;
    }
    if (yyjson_is_str(node)) {
        *out = JsonKind_String;
        return true;
    }
    if (yyjson_is_arr(node)) {
        *out = JsonKind_Array;
        return true;
    }
    if (yyjson_is_obj(node)) {
        *out = JsonKind_Object;
        return true;
    }
    return false;
}

bool Json_Bool(Json value, bool *out)
{
    const yyjson_val *node = json_node(value);

    if (out == NULL || node == NULL) {
        SDL_Log("Json_Bool expected a JSON value and output, received NULL");
        return false;
    }
    if (!yyjson_is_bool(node)) {
        return false;
    }
    *out = yyjson_get_bool(node);
    return true;
}

bool Json_Text(Json value, Utf8 *out)
{
    const yyjson_val *node = json_node(value);
    const char *data = NULL;
    size_t length = 0u;

    if (out == NULL || node == NULL) {
        SDL_Log("Json_Text expected a JSON value and output, received NULL");
        return false;
    }
    if (yyjson_is_str(node)) {
        data = yyjson_get_str(node);
    } else if (yyjson_is_raw(node)) {
        data = yyjson_get_raw(node);
    } else {
        return false;
    }
    if (data == NULL) {
        return false;
    }
    length = yyjson_get_len(node);
    return Utf8_Borrow(data, (Uint64)length, out);
}

bool Json_Count(Json value, Uint64 *out)
{
    const yyjson_val *node = json_node(value);

    if (out == NULL || node == NULL) {
        SDL_Log("Json_Count expected a JSON value and output, received NULL");
        return false;
    }
    if (yyjson_is_arr(node)) {
        *out = (Uint64)yyjson_arr_size(node);
        return true;
    }
    if (yyjson_is_obj(node)) {
        *out = (Uint64)yyjson_obj_size(node);
        return true;
    }
    return false;
}

bool Json_Element(Json value, Uint64 index, Json *out)
{
    const yyjson_val *node = json_node(value);
    yyjson_val *element = NULL;
    size_t count = 0u;

    if (out == NULL || node == NULL) {
        SDL_Log("Json_Element expected a JSON array and output, received NULL");
        return false;
    }
    if (!yyjson_is_arr(node) || !size_fits(index)) {
        return false;
    }
    count = yyjson_arr_size(node);
    if (index >= (Uint64)count) {
        return false;
    }
    element = yyjson_arr_get(node, size_cast(index));
    if (element == NULL) {
        return false;
    }
    *out = json_from(element);
    return true;
}

bool Json_Pair(Json value, Uint64 index, Json *key, Json *pair)
{
    const yyjson_val *node = json_node(value);
    yyjson_val *current = NULL;
    yyjson_val *stored = NULL;
    size_t count = 0u;
    size_t idx = 0u;

    if (key == NULL || pair == NULL || node == NULL) {
        SDL_Log("Json_Pair expected a JSON object and outputs, received NULL");
        return false;
    }
    if (!yyjson_is_obj(node) || !size_fits(index)) {
        return false;
    }
    count = yyjson_obj_size(node);
    if (index >= (Uint64)count) {
        return false;
    }
    current = unsafe_yyjson_get_first(node);
    for (idx = 0u; idx < size_cast(index); idx += 1u) {
        stored = current + 1;
        current = unsafe_yyjson_get_next(stored);
    }
    stored = current + 1;
    *key = json_from(current);
    *pair = json_from(stored);
    return true;
}

bool Json_Field(Json value, Utf8 key, Json *out)
{
    const yyjson_val *node = json_node(value);
    yyjson_val *stored = NULL;

    if (out == NULL || node == NULL || !usable_text(key)) {
        SDL_Log("Json_Field expected a JSON object, key, and output");
        return false;
    }
    if (!yyjson_is_obj(node)) {
        return false;
    }
    stored = yyjson_obj_getn(node, key.data, size_cast(key.size));
    if (stored == NULL) {
        return false;
    }
    *out = json_from(stored);
    return true;
}
