/**
 * @file arena.c
 * @brief One-owner bump allocator backed by a single SDL allocation.
 */

#include "ascend_forge/arena.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/** Bytes reserved for the header before the caller's region. */
static const Uint64 k_arena_header_align = 16u;

/** Address returned when an allocation cannot be satisfied. Do not write it. */
static Uint8 g_arena_empty_byte = 0;

struct Arena {
    Uint8 *base;
    Uint64 capacity;
    Uint64 used;
    bool owns_memory;
};

static Arena g_arena_sentinel = {
    &g_arena_empty_byte,
    0u,
    0u,
    false,
};

/**
 * @brief Round value up to a power-of-two alignment.
 * @param overflow Set when the sum would wrap.
 */
static Uint64 arena_align_up(Uint64 value, Uint64 alignment, bool *overflow)
{
    const Uint64 mask = alignment - 1u;

    if (value > (~(Uint64)0) - mask) {
        *overflow = true;
        return 0u;
    }
    *overflow = false;
    return (value + mask) & ~mask;
}

static bool arena_is_power_of_two(Uint32 alignment)
{
    return (alignment != 0u) && ((alignment & (alignment - 1u)) == 0u) ? true : false;
}

static void arena_fail_out(void **out)
{
    if (out != NULL) {
        *out = &g_arena_empty_byte;
    }
}

bool Arena_Create(Uint64 capacity, Arena **out)
{
    Uint64 header = 0u;
    Uint64 total = 0u;
    bool overflow = false;
    void *block = NULL;
    Arena *arena = NULL;

    if (out == NULL) {
        SDL_Log("Arena_Create expected an out parameter, received NULL");
        return false;
    }
    if (capacity == 0u) {
        SDL_Log("Arena_Create expected capacity > 0, received 0");
        *out = &g_arena_sentinel;
        return false;
    }

    header = arena_align_up((Uint64)sizeof(Arena), k_arena_header_align, &overflow);
    if (overflow || capacity > (~(Uint64)0) - header) {
        SDL_Log("Arena_Create expected a capacity that fits in one allocation, received %llu",
                (unsigned long long)capacity);
        *out = &g_arena_sentinel;
        return false;
    }
    total = header + capacity;
    if (total > (Uint64)SIZE_MAX) {
        SDL_Log("Arena_Create expected a capacity that fits in size_t, received %llu",
                (unsigned long long)capacity);
        *out = &g_arena_sentinel;
        return false;
    }

    block = SDL_malloc((size_t)total);
    if (block == NULL) {
        SDL_Log("Arena_Create expected %llu bytes from SDL_malloc, received none",
                (unsigned long long)total);
        exit(EXIT_FAILURE);
    }

    arena = block;
    arena->base = (Uint8 *)block + (size_t)header;
    arena->capacity = capacity;
    arena->used = 0u;
    arena->owns_memory = true;
    *out = arena;
    return true;
}

void Arena_Destroy(Arena *arena)
{
    if (arena == NULL) {
        SDL_Log("Arena_Destroy expected an arena, received NULL");
        return;
    }
    if (!arena->owns_memory) {
        return;
    }
    SDL_free(arena);
}

bool Arena_Alloc(Arena *arena, Uint64 size, Uint32 alignment, void **out)
{
    bool overflow = false;
    Uint64 base_address = 0u;
    Uint64 absolute = 0u;
    Uint64 offset = 0u;

    if (out == NULL) {
        SDL_Log("Arena_Alloc expected an out parameter, received NULL");
        return false;
    }
    if (arena == NULL) {
        SDL_Log("Arena_Alloc expected an arena, received NULL");
        arena_fail_out(out);
        return false;
    }
    if (!arena_is_power_of_two(alignment)) {
        SDL_Log("Arena_Alloc expected a power-of-two alignment, received %u",
                (unsigned int)alignment);
        arena_fail_out(out);
        return false;
    }
    if (size == 0u) {
        *out = &g_arena_empty_byte;
        return true;
    }
    if (arena->capacity == 0u || arena->base == NULL) {
        SDL_Log("Arena_Alloc expected a live arena, received a sentinel");
        arena_fail_out(out);
        return false;
    }

    base_address = (Uint64)(uintptr_t)arena->base;
    if (arena->used > (~(Uint64)0) - base_address) {
        SDL_Log("Arena_Alloc expected a used count that fits in an address, received %llu",
                (unsigned long long)arena->used);
        arena_fail_out(out);
        return false;
    }
    absolute = arena_align_up(base_address + arena->used, (Uint64)alignment, &overflow);
    if (overflow || absolute < base_address) {
        SDL_Log("Arena_Alloc expected alignment %u to fit, received used %llu",
                (unsigned int)alignment,
                (unsigned long long)arena->used);
        arena_fail_out(out);
        return false;
    }
    offset = absolute - base_address;
    if (offset > arena->capacity || size > arena->capacity - offset) {
        SDL_Log("Arena_Alloc expected %llu bytes with %llu remaining, received a short arena",
                (unsigned long long)size,
                (unsigned long long)(arena->capacity - arena->used));
        arena_fail_out(out);
        return false;
    }

    arena->used = offset + size;
    *out = arena->base + (size_t)offset;
    return true;
}

void Arena_Reset(Arena *arena)
{
    if (arena == NULL) {
        SDL_Log("Arena_Reset expected an arena, received NULL");
        return;
    }
    arena->used = 0u;
}

Uint64 Arena_Mark(const Arena *arena)
{
    if (arena == NULL) {
        SDL_Log("Arena_Mark expected an arena, received NULL");
        return 0u;
    }
    return arena->used;
}

bool Arena_Rewind(Arena *arena, Uint64 mark)
{
    if (arena == NULL) {
        SDL_Log("Arena_Rewind expected an arena, received NULL");
        return false;
    }
    if (mark > arena->used) {
        SDL_Log("Arena_Rewind expected a mark <= %llu, received %llu",
                (unsigned long long)arena->used,
                (unsigned long long)mark);
        return false;
    }
    arena->used = mark;
    return true;
}

Uint64 Arena_Used(const Arena *arena)
{
    if (arena == NULL) {
        SDL_Log("Arena_Used expected an arena, received NULL");
        return 0u;
    }
    return arena->used;
}

Uint64 Arena_Capacity(const Arena *arena)
{
    if (arena == NULL) {
        SDL_Log("Arena_Capacity expected an arena, received NULL");
        return 0u;
    }
    return arena->capacity;
}
