/**
 * @file json.h
 * @brief Immutable JSON values in the caller's arena.
 *
 * Json_Parse reads one JSON value into the arena. Strings and numbers are
 * Utf8 spans. A number keeps the bytes from the source. Arrays and objects
 * are contiguous ranges. Stringify appends text to a Utf8Builder.
 *
 * Invalid JSON returns a Result_ and leaves the output value untouched.
 * The arena space used by the attempt stays consumed. Out of memory from
 * SDL_malloc stays fatal inside the arena and is not a result. An arena
 * that cannot hold the value returns a result.
 *
 * One writer runs during the parse. After it returns, any job may read the
 * tree. The borrowed text stays valid for as long as that arena does.
 * A builder is not shared.
 */

#ifndef ASCEND_FORGE_JSON_H
#define ASCEND_FORGE_JSON_H

#include "ascend_forge/result.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * One JSON value. node is private and is never NULL after a successful
 * call that writes this value. Callers use the Json_ accessors.
 */
typedef struct Json {
    const void *node;
} Json;

/** Kind of one JSON value. */
typedef enum JsonKind {
    JsonKind_Null = 1,
    JsonKind_Bool,
    JsonKind_Number,
    JsonKind_String,
    JsonKind_Array,
    JsonKind_Object
} JsonKind;

/** Failure code stored in a Result from Json_. Zero remains success. */
typedef enum JsonCode {
    JsonCode_Invalid = 1
} JsonCode;

/**
 * @brief Parse one JSON value into the arena.
 *
 * Numbers stay source text. Trailing bytes are an error. The read does not
 * modify text.
 *
 * @param arena Receives the value tree. A short arena returns a result.
 * @param text JSON text. It does not need a NUL terminator.
 * @param out Receives the root value. On failure this is left untouched.
 * @param result Receives success or the failure. A NULL result is logged
 *        and is not written.
 * @return false when the text is not one JSON value, the arena cannot hold
 *         it, or an argument is unusable.
 */
bool Json_Parse(Arena *arena, Utf8 text, Json *out, Result *result);

/**
 * @brief Append the JSON text for value to the builder.
 *
 * The temporary text is allocated from the builder's arena. A private
 * arena can hold value while the builder lives in another arena. Failure
 * leaves the builder unchanged.
 *
 * @param value A value returned by Json_Parse or an accessor.
 * @param out Builder that receives the text. It has one owner.
 * @param result Receives success or the failure.
 * @return false when the value cannot be written or an argument is unusable.
 */
bool Json_Stringify(Json value, Utf8Builder *out, Result *result);

/**
 * @brief Write the kind of value.
 *
 * @param out Receives the kind. On failure this is left untouched.
 * @return false when value is unusable or out is NULL.
 */
bool Json_Kind(Json value, JsonKind *out);

/**
 * @brief Write the bool stored in value.
 *
 * @param out Receives true or false. On failure this is left untouched.
 * @return false when value is not a bool or an argument is unusable.
 */
bool Json_Bool(Json value, bool *out);

/**
 * @brief Borrow the text of a string or the source bytes of a number.
 *
 * @param out Receives the span. On failure this is left untouched. An empty
 *        string is a real pointer and a length of zero.
 * @return false when value is neither a string nor a number, or an argument
 *         is unusable.
 */
bool Json_Text(Json value, Utf8 *out);

/**
 * @brief Write the number of array elements or object pairs.
 *
 * @param out Receives the count. On failure this is left untouched.
 * @return false when value is not an array or object, or an argument is
 *         unusable.
 */
bool Json_Count(Json value, Uint64 *out);

/**
 * @brief Write the array element at index.
 *
 * @param out Receives the element. On failure this is left untouched.
 * @return false when value is not an array, index is out of range, or an
 *         argument is unusable.
 */
bool Json_Element(Json value, Uint64 index, Json *out);

/**
 * @brief Write the object key and value at index.
 *
 * Pairs keep source order. The key is a string value.
 *
 * @param key Receives the key. On failure this is left untouched.
 * @param pair Receives the value. On failure this is left untouched.
 * @return false when value is not an object, index is out of range, or an
 *         argument is unusable.
 */
bool Json_Pair(Json value, Uint64 index, Json *key, Json *pair);

/**
 * @brief Write the first object value stored under key.
 *
 * A missing key leaves out untouched. A stored null is a successful null
 * value. Duplicate keys keep their source order. This returns the first.
 *
 * @param key Field name. It does not need a NUL terminator.
 * @param out Receives the value. On failure this is left untouched.
 * @return false when value is not an object, the key is absent, or an
 *         argument is unusable.
 */
bool Json_Field(Json value, Utf8 key, Json *out);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_JSON_H */
