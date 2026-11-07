/**
 * @file slice.c
 * @brief Growable contiguous collection backed by the caller's arena.
 */

#include "ascend_forge/slice.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

/** Storage for an empty slice. Callers must not write it. */
alignas(64) static Uint8 g_slice_empty[64];

static bool slice_fits(Uint64 size)
{
    return size <= (Uint64)SIZE_MAX;
}

static size_t slice_cast(Uint64 size)
{
    return (size_t)size;
}

static bool slice_product(Uint64 count, Uint64 size, Uint64 *out)
{
    if (size != 0u && count > (~(Uint64)0) / size) {
        return false;
    }
    *out = count * size;
    return true;
}

static bool slice_power_of_two(Uint64 alignment)
{
    return (alignment != 0u) && ((alignment & (alignment - 1u)) == 0u) ? true : false;
}

static bool slice_copy_at(void *base, Uint64 index, Uint64 size, const void *element)
{
    Uint64 offset = 0u;

    if (!slice_product(index, size, &offset) || !slice_fits(offset) || !slice_fits(size)) {
        return false;
    }
    SDL_memcpy((Uint8 *)base + slice_cast(offset), element, slice_cast(size));
    return true;
}

static bool slice_next_capacity(Uint64 capacity, Uint64 count, Uint64 *out)
{
    Uint64 grown = 0u;

    if (count == ~(Uint64)0) {
        return false;
    }
    if (capacity == 0u) {
        *out = 1u;
        return true;
    }
    if (capacity > (~(Uint64)0) / 2u) {
        grown = ~(Uint64)0;
    } else {
        grown = capacity * 2u;
    }
    if (grown < count + 1u) {
        grown = count + 1u;
    }
    *out = grown;
    return true;
}

bool Slice_Init(void **out)
{
    if (out == NULL) {
        SDL_Log("Slice_Init expected an out parameter, received NULL");
        return false;
    }
    *out = g_slice_empty;
    return true;
}

bool Slice_Append(void **data, Uint64 *count, Uint64 *capacity, Arena *arena, const void *element,
                  Uint64 size, Uint64 alignment)
{
    Uint64 live = 0u;
    Uint64 room = 0u;
    Uint64 grown = 0u;
    Uint64 bytes = 0u;
    Uint64 old_bytes = 0u;
    void *block = NULL;

    if (data == NULL || *data == NULL || count == NULL || capacity == NULL) {
        SDL_Log("Slice_Append expected a slice with a real pointer");
        return false;
    }
    if (arena == NULL) {
        SDL_Log("Slice_Append expected an arena, received NULL");
        return false;
    }
    if (element == NULL) {
        SDL_Log("Slice_Append expected an element, received NULL");
        return false;
    }
    if (size == 0u) {
        SDL_Log("Slice_Append expected an element size greater than 0, received 0");
        return false;
    }
    if (!slice_power_of_two(alignment) || alignment > (Uint64)UINT32_MAX) {
        SDL_Log("Slice_Append expected a power-of-two alignment, received %llu",
                (unsigned long long)alignment);
        return false;
    }

    live = *count;
    room = *capacity;
    if (live > room) {
        SDL_Log("Slice_Append expected count <= capacity, received %llu and %llu",
                (unsigned long long)live,
                (unsigned long long)room);
        return false;
    }
    if (live < room) {
        if (!slice_copy_at(*data, live, size, element)) {
            SDL_Log("Slice_Append expected a byte count that fits, received count %llu",
                    (unsigned long long)live);
            return false;
        }
        *count = live + 1u;
        return true;
    }
    if (!slice_next_capacity(room, live, &grown) || !slice_product(grown, size, &bytes) ||
        !slice_product(live, size, &old_bytes) || !slice_fits(bytes) || !slice_fits(old_bytes)) {
        SDL_Log("Slice_Append expected a byte count that fits, received count %llu",
                (unsigned long long)live);
        return false;
    }
    if (!Arena_Alloc(arena, bytes, (Uint32)alignment, &block)) {
        return false;
    }
    if (live > 0u) {
        SDL_memcpy(block, *data, slice_cast(old_bytes));
    }
    SDL_memcpy((Uint8 *)block + slice_cast(old_bytes), element, slice_cast(size));
    *data = block;
    *count = live + 1u;
    *capacity = grown;
    return true;
}

bool Slice_Remove(void *data, Uint64 *count, Uint64 index, Uint64 size)
{
    Uint64 live = 0u;
    Uint64 tail = 0u;
    Uint64 offset = 0u;
    Uint64 tail_bytes = 0u;
    Uint8 *base = NULL;

    if (data == NULL || count == NULL) {
        SDL_Log("Slice_Remove expected a slice with a real pointer");
        return false;
    }
    if (size == 0u) {
        SDL_Log("Slice_Remove expected an element size greater than 0, received 0");
        return false;
    }
    live = *count;
    if (index >= live) {
        SDL_Log("Slice_Remove expected an index below %llu, received %llu",
                (unsigned long long)live,
                (unsigned long long)index);
        return false;
    }
    tail = live - index - 1u;
    if (tail > 0u) {
        if (!slice_product(index, size, &offset) || !slice_product(tail, size, &tail_bytes) ||
            !slice_fits(offset) || !slice_fits(tail_bytes) || !slice_fits(size)) {
            SDL_Log("Slice_Remove expected a byte count that fits, received index %llu",
                    (unsigned long long)index);
            return false;
        }
        base = (Uint8 *)data;
        SDL_memmove(base + slice_cast(offset), base + slice_cast(offset) + slice_cast(size),
                    slice_cast(tail_bytes));
    }
    *count = live - 1u;
    return true;
}
