/**
 * @file atomic.c
 * @brief SDL atomics for a completion count and a published pointer.
 */

#include "ascend_forge/atomic.h"

bool AtomicU32_Init(AtomicU32 *atomic, Uint32 value)
{
    if (atomic == NULL) {
        SDL_Log("AtomicU32_Init expected a counter, received NULL");
        return false;
    }
    (void)SDL_SetAtomicU32(&atomic->value, value);
    return true;
}

bool AtomicU32_Load(AtomicU32 *atomic, Uint32 *out)
{
    if (atomic == NULL || out == NULL) {
        SDL_Log("AtomicU32_Load expected a counter and an out parameter");
        return false;
    }
    *out = SDL_GetAtomicU32(&atomic->value);
    return true;
}

bool AtomicU32_Store(AtomicU32 *atomic, Uint32 value, Uint32 *previous)
{
    if (atomic == NULL || previous == NULL) {
        SDL_Log("AtomicU32_Store expected a counter and a previous value");
        return false;
    }
    *previous = SDL_SetAtomicU32(&atomic->value, value);
    return true;
}

bool AtomicU32_Add(AtomicU32 *atomic, Sint32 delta, Uint32 *previous)
{
    if (atomic == NULL || previous == NULL) {
        SDL_Log("AtomicU32_Add expected a counter and a previous value");
        return false;
    }
    *previous = SDL_AddAtomicU32(&atomic->value, (int)delta);
    return true;
}

bool AtomicU32_CompareSwap(AtomicU32 *atomic, Uint32 expected, Uint32 desired)
{
    if (atomic == NULL) {
        SDL_Log("AtomicU32_CompareSwap expected a counter, received NULL");
        return false;
    }
    return SDL_CompareAndSwapAtomicU32(&atomic->value, expected, desired);
}

bool AtomicPointer_Init(void **slot)
{
    if (slot == NULL) {
        SDL_Log("AtomicPointer_Init expected a slot, received NULL");
        return false;
    }
    (void)SDL_SetAtomicPointer(slot, NULL);
    return true;
}

bool Atomic_Publish(void **slot, void *value)
{
    if (slot == NULL) {
        SDL_Log("Atomic_Publish expected a slot, received NULL");
        return false;
    }
    if (value == NULL) {
        SDL_Log("Atomic_Publish expected a finished value, received NULL");
        return false;
    }
    SDL_MemoryBarrierRelease();
    (void)SDL_SetAtomicPointer(slot, value);
    return true;
}

bool Atomic_Acquire(void **slot, void **out)
{
    void *value = NULL;

    if (slot == NULL || out == NULL) {
        SDL_Log("Atomic_Acquire expected a slot and an out parameter");
        return false;
    }
    value = SDL_GetAtomicPointer(slot);
    if (value == NULL) {
        return false;
    }
    SDL_MemoryBarrierAcquire();
    *out = value;
    return true;
}
