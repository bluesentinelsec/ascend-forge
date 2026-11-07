/**
 * @file time.h
 * @brief Monotonic nanosecond clock.
 *
 * Time_Now reads SDL_GetTicksNS. The value does not go backward for the life
 * of the process. It is not a wall clock. The call does not allocate and does
 * not start a thread.
 */

#ifndef ASCEND_FORGE_TIME_H
#define ASCEND_FORGE_TIME_H

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read the monotonic nanosecond clock.
 *
 * The value is the time since the clock started, in nanoseconds. Two reads
 * in one process return a second value greater than or equal to the first.
 *
 * @return The current monotonic time in nanoseconds.
 */
Uint64 Time_Now(void);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_TIME_H */
