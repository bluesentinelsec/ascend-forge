/**
 * @file arena.h
 * @brief Bump allocator owned by one caller.
 *
 * The backing block is one SDL_malloc. Jobs do not share an arena. A value
 * in the arena is writable by that owner until the arena is reset, rewound,
 * or destroyed.
 */

#ifndef ASCEND_FORGE_ARENA_H
#define ASCEND_FORGE_ARENA_H

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque bump allocator. */
typedef struct Arena Arena;

/**
 * @brief Create an arena with a fixed capacity.
 *
 * @param capacity Number of bytes available to Arena_Alloc. Must be greater
 *        than zero.
 * @param out Receives the arena. On failure this is a non-NULL sentinel that
 *        Arena_Destroy accepts and Arena_Alloc rejects.
 * @return true when the arena can allocate. false when capacity is 0,
 *         capacity does not fit in one allocation, or out is NULL.
 *
 * The backing SDL_malloc failure logs and exits. It is not reported here.
 */
bool Arena_Create(Uint64 capacity, Arena **out);

/**
 * @brief Free an arena created by Arena_Create.
 *
 * Destroying the failure sentinel is a no-op. A NULL arena is logged and
 * ignored.
 */
void Arena_Destroy(Arena *arena);

/**
 * @brief Allocate a contiguous range from the arena.
 *
 * @param arena Arena returned by Arena_Create, including the failure sentinel.
 * @param size Number of bytes. Zero succeeds and does not consume capacity.
 * @param alignment Power of two. The returned address is aligned to it.
 * @param out Receives the address. On failure this is a non-NULL byte that
 *        the caller must not write.
 * @return true on success. false when the request does not fit, the
 *         alignment is not a power of two, or an argument is unusable. A
 *         failure leaves the arena's used count unchanged.
 */
bool Arena_Alloc(Arena *arena, Uint64 size, Uint32 alignment, void **out);

/**
 * @brief Drop every allocation. The backing block stays.
 */
void Arena_Reset(Arena *arena);

/**
 * @brief Return the current used-byte mark for Arena_Rewind.
 *
 * @return The mark, or 0 when arena is unusable.
 */
Uint64 Arena_Mark(const Arena *arena);

/**
 * @brief Restore the arena to a mark from Arena_Mark.
 *
 * @param mark A value previously returned by Arena_Mark on this arena, and
 *        not greater than the current used count.
 * @return false when the mark is past the used count. The arena is
 *         unchanged in that case.
 */
bool Arena_Rewind(Arena *arena, Uint64 mark);

/**
 * @brief Bytes currently reserved by allocations and alignment padding.
 */
Uint64 Arena_Used(const Arena *arena);

/**
 * @brief Bytes Arena_Alloc may draw from. This does not include the header.
 */
Uint64 Arena_Capacity(const Arena *arena);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_ARENA_H */
