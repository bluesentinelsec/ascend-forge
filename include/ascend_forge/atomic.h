/**
 * @file atomic.h
 * @brief Completion counts and a release publish of a finished value.
 *
 * A counter is an address that any thread may update after init. Callers
 * share that address. They do not copy the counter. Init happens before any
 * other caller can reach it.
 *
 * A finished value is published through one pointer slot. The unpublished
 * slot is a null pointer, and that null stays inside this primitive. The
 * caller writes the value, then publishes its pointer. A reader acquires the
 * pointer before it reads the value.
 */

#ifndef ASCEND_FORGE_ATOMIC_H
#define ASCEND_FORGE_ATOMIC_H

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Atomic completion count. Share the address, not a copy. */
typedef struct AtomicU32 {
    SDL_AtomicU32 value;
} AtomicU32;

/**
 * @brief Store value before any other caller can reach the counter.
 *
 * @param atomic Counter to initialize.
 * @param value Initial count.
 * @return false when atomic is NULL.
 */
bool AtomicU32_Init(AtomicU32 *atomic, Uint32 value);

/**
 * @brief Read the current count.
 *
 * @param out Receives the count. On failure this is left untouched.
 * @return false when an argument is NULL.
 */
bool AtomicU32_Load(AtomicU32 *atomic, Uint32 *out);

/**
 * @brief Replace the count.
 *
 * @param previous Receives the count before the store. On failure this is
 *        left untouched.
 * @return false when an argument is NULL.
 */
bool AtomicU32_Store(AtomicU32 *atomic, Uint32 value, Uint32 *previous);

/**
 * @brief Add delta to the count.
 *
 * The returned previous value is the count before the add. A completion
 * count that moves from 1 to 0 reports a previous value of 1.
 *
 * @param delta Amount to add. A negative value subtracts.
 * @param previous Receives the count before the add. On failure this is
 *        left untouched.
 * @return false when an argument is NULL.
 */
bool AtomicU32_Add(AtomicU32 *atomic, Sint32 delta, Uint32 *previous);

/**
 * @brief Store desired when the count is still expected.
 *
 * A mismatch leaves the count as it was and returns false. A NULL counter
 * is logged and also returns false.
 */
bool AtomicU32_CompareSwap(AtomicU32 *atomic, Uint32 expected, Uint32 desired);

/**
 * @brief Make slot unpublished.
 *
 * The unpublished value is a null pointer stored by this function. The
 * caller does not write that null.
 *
 * @return false when slot is NULL.
 */
bool AtomicPointer_Init(void **slot);

/**
 * @brief Publish a finished value.
 *
 * The caller finishes every write to that value before this call. This
 * applies a release barrier and then stores the pointer.
 *
 * @param slot Slot from AtomicPointer_Init.
 * @param value Finished value. NULL is rejected and the slot stays as it was.
 * @return false when an argument is unusable.
 */
bool Atomic_Publish(void **slot, void *value);

/**
 * @brief Read a published pointer.
 *
 * On success the acquire barrier has run and out receives the pointer. The
 * caller may then read the value. An unpublished slot returns false and
 * leaves out untouched.
 *
 * @return false when the slot is unpublished or an argument is NULL.
 */
bool Atomic_Acquire(void **slot, void **out);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_ATOMIC_H */
