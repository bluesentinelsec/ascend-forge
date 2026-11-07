/**
 * @file fork.c
 * @brief Fork-join over one index range.
 */

#include "ascend_forge/fork.h"

#include "ascend_forge/atomic.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct ForkJoinSlot {
    IndexRange range;
    IndexRangeFn fn;
    void *data;
    AtomicU32 *remaining;
} ForkJoinSlot;

static void *forkJoin_alloc(Uint32 count, size_t element)
{
    void *memory = NULL;

    if (element == 0u || (size_t)count > (SIZE_MAX / element)) {
        SDL_Log("ForkJoin_Run expected a worker count that fits, received %" SDL_PRIu32, count);
        exit(EXIT_FAILURE);
    }
    memory = SDL_malloc((size_t)count * element);
    if (memory == NULL) {
        SDL_Log("ForkJoin_Run expected %" SDL_PRIu32 " records from SDL_malloc, received none", count);
        exit(EXIT_FAILURE);
    }
    return memory;
}

#ifndef __EMSCRIPTEN__
static int SDLCALL forkJoin_worker(void *data)
{
    ForkJoinSlot *slot = NULL;
    Uint32 previous = 0u;

    slot = (ForkJoinSlot *)data;
    if (slot == NULL || slot->fn == NULL || slot->remaining == NULL) {
        return 1;
    }
    if (!IndexRange_ForEach(&slot->range, slot->fn, slot->data)) {
        (void)AtomicU32_Add(slot->remaining, -1, &previous);
        return 2;
    }
    if (!AtomicU32_Add(slot->remaining, -1, &previous)) {
        return 3;
    }
    return 0;
}
#endif

bool ForkJoin_Run(Thread *workers, Uint32 worker_count, const IndexRange *range, IndexRangeFn fn, void *data)
{
    IndexRange *parts = NULL;
    ForkJoinSlot *slots = NULL;
    AtomicU32 remaining = {{0u}};
    Uint32 index = 0u;
    Uint32 left = 1u;
    bool ok = true;

    if (workers == NULL || range == NULL || fn == NULL || worker_count == 0u) {
        SDL_Log("ForkJoin_Run expected workers, a range, a function, and a worker count "
                "greater than zero");
        return false;
    }
    if (range->end < range->begin) {
        SDL_Log("ForkJoin_Run expected an end at or after begin %" SDL_PRIu32 ", received %" SDL_PRIu32,
                range->begin, range->end);
        return false;
    }

    parts = (IndexRange *)forkJoin_alloc(worker_count, sizeof(IndexRange));
    slots = (ForkJoinSlot *)forkJoin_alloc(worker_count, sizeof(ForkJoinSlot));
    if (!IndexRange_Split(range, parts, worker_count) || !AtomicU32_Init(&remaining, worker_count)) {
        SDL_free(parts);
        SDL_free(slots);
        return false;
    }
    for (index = 0u; index < worker_count; index += 1u) {
        slots[index].range = parts[index];
        slots[index].fn = fn;
        slots[index].data = data;
        slots[index].remaining = &remaining;
    }

#ifdef __EMSCRIPTEN__
    for (index = 0u; index < worker_count; index += 1u) {
        Uint32 previous = 0u;

        if (!IndexRange_ForEach(&slots[index].range, slots[index].fn, slots[index].data)) {
            ok = false;
        }
        if (!AtomicU32_Add(&remaining, -1, &previous)) {
            ok = false;
        }
    }
#else
    {
        Uint32 started = 0u;

        for (index = 0u; index < worker_count; index += 1u) {
            if (!Thread_Init(&workers[index])) {
                ok = false;
                break;
            }
        }
        if (ok) {
            for (index = 0u; index < worker_count; index += 1u) {
                if (!Thread_Run(&workers[index], forkJoin_worker, "fork", &slots[index])) {
                    SDL_Log("ForkJoin_Run failed to start worker %" SDL_PRIu32 ": %s", index,
                            SDL_GetError());
                    ok = false;
                    break;
                }
                started += 1u;
            }
        }
        for (index = 0u; index < started; index += 1u) {
            Sint32 status = 0;

            if (!Thread_Join(&workers[index], &status) || status != 0) {
                ok = false;
            }
        }
    }
#endif

    if (!AtomicU32_Load(&remaining, &left) || left != 0u) {
        ok = false;
    }
    SDL_free(parts);
    SDL_free(slots);
    return ok;
}
