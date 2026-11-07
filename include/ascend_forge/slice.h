/**
 * @file slice.h
 * @brief Growable contiguous collection in the caller's arena.
 *
 * A slice is a typed pointer, a count, and a capacity. The bytes are one
 * range in the caller's arena. The slice does not call SDL_malloc. One owner
 * appends and removes. Jobs may read a finished slice. They do not share a
 * slice that can still grow.
 *
 * C has no templates. Slice_Define stamps one element type. Call sites then
 * use Slice_<Name>_Init, Slice_<Name>_Append, and Slice_<Name>_Remove.
 */

#ifndef ASCEND_FORGE_SLICE_H
#define ASCEND_FORGE_SLICE_H

#include "ascend_forge/arena.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Point out at the empty slice storage.
 *
 * The empty slice is a real pointer and a length of zero. This does not
 * allocate. The caller must not write through that pointer.
 *
 * @param out Receives the empty pointer. On failure this is left untouched.
 * @return false when out is NULL.
 */
bool Slice_Init(void **out);

/**
 * @brief Append one element, growing into a new arena block when needed.
 *
 * When count is below capacity, the element is copied into the current
 * block. Otherwise this allocates a new block, copies the live elements,
 * and appends there. The old block stays in the arena. An append that does
 * not fit leaves every output unchanged. SDL_malloc failure stays fatal
 * inside the arena.
 *
 * @param data Pointer to the slice's element storage. Never NULL on entry
 *        after Slice_Init.
 * @param count Pointer to the live element count.
 * @param capacity Pointer to the element capacity of the current block.
 * @param arena Arena that owns the bytes. Required even when the current
 *        block still has room.
 * @param element Element bytes to copy. size is the element size.
 * @param size Element size in bytes. Must be greater than zero.
 * @param alignment Element alignment. It must be a power of two and fit in
 *        the arena's alignment argument.
 * @return false when the element does not fit or an argument is unusable.
 */
bool Slice_Append(void **data, Uint64 *count, Uint64 *capacity, Arena *arena,
                  const void *element, Uint64 size, Uint64 alignment);

/**
 * @brief Remove the element at index and compact the range.
 *
 * The arena keeps the bytes. Capacity and the storage pointer stay as they
 * were. A rejected index leaves count unchanged.
 *
 * @param data Element storage from Slice_Init or Slice_Append.
 * @param count Pointer to the live element count.
 * @param index Index of the element to remove.
 * @param size Element size in bytes. Must be greater than zero.
 * @return false when index is past the end or an argument is unusable.
 */
bool Slice_Remove(void *data, Uint64 *count, Uint64 index, Uint64 size);

#if defined(_MSC_VER)
#define SLICE_WARN_PUSH __pragma(warning(push)) __pragma(warning(disable : 4505))
#define SLICE_WARN_POP __pragma(warning(pop))
#define SLICE_MAYBE_UNUSED
#elif defined(__GNUC__) || defined(__clang__)
#define SLICE_WARN_PUSH
#define SLICE_WARN_POP
#define SLICE_MAYBE_UNUSED __attribute__((unused))
#else
#define SLICE_WARN_PUSH
#define SLICE_WARN_POP
#define SLICE_MAYBE_UNUSED
#endif

/**
 * @brief Stamp a typed slice named Slice_<Name>.
 *
 * Element is one type name. The stamped struct holds Element *data, a count,
 * and a capacity. Slice_<Name>_Init, Slice_<Name>_Append, and
 * Slice_<Name>_Remove follow Slice_Init, Slice_Append, and Slice_Remove.
 * Append uses Element's size and alignment.
 */
#define Slice_Define(Name, Element)                                                                \
    SLICE_WARN_PUSH                                                                                \
    typedef struct Slice_##Name {                                                                  \
        Element *data;                                                                             \
        Uint64 count;                                                                              \
        Uint64 capacity;                                                                           \
    } Slice_##Name;                                                                                \
                                                                                                   \
    static inline SLICE_MAYBE_UNUSED bool Slice_##Name##_Init(Slice_##Name *out)                   \
    {                                                                                              \
        void *data = NULL;                                                                         \
                                                                                                   \
        if (out == NULL) {                                                                         \
            return Slice_Init(NULL);                                                               \
        }                                                                                          \
        if (!Slice_Init(&data)) {                                                                  \
            return false;                                                                          \
        }                                                                                          \
        out->data = (Element *)data;                                                               \
        out->count = 0u;                                                                           \
        out->capacity = 0u;                                                                        \
        return true;                                                                               \
    }                                                                                              \
                                                                                                   \
    static inline SLICE_MAYBE_UNUSED bool Slice_##Name##_Append(Slice_##Name *slice, Arena *arena, \
                                                                Element value)                     \
    {                                                                                              \
        void *data = NULL;                                                                         \
        Uint64 count = 0u;                                                                         \
        Uint64 capacity = 0u;                                                                      \
                                                                                                   \
        if (slice == NULL) {                                                                       \
            return Slice_Append(NULL, NULL, NULL, arena, &value, (Uint64)sizeof(Element),          \
                                (Uint64)_Alignof(Element));                                        \
        }                                                                                          \
        data = slice->data;                                                                        \
        count = slice->count;                                                                      \
        capacity = slice->capacity;                                                                \
        if (!Slice_Append(&data, &count, &capacity, arena, &value, (Uint64)sizeof(Element),        \
                          (Uint64)_Alignof(Element))) {                                            \
            return false;                                                                          \
        }                                                                                          \
        slice->data = (Element *)data;                                                             \
        slice->count = count;                                                                      \
        slice->capacity = capacity;                                                                \
        return true;                                                                               \
    }                                                                                              \
                                                                                                   \
    static inline SLICE_MAYBE_UNUSED bool Slice_##Name##_Remove(Slice_##Name *slice, Uint64 index) \
    {                                                                                              \
        if (slice == NULL) {                                                                       \
            return Slice_Remove(NULL, NULL, index, (Uint64)sizeof(Element));                       \
        }                                                                                          \
        return Slice_Remove(slice->data, &slice->count, index, (Uint64)sizeof(Element));           \
    }                                                                                              \
    SLICE_WARN_POP

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_SLICE_H */
