/**
 * @file time.c
 * @brief SDL monotonic nanosecond clock.
 */

#include "ascend_forge/time.h"

Uint64 Time_Now(void)
{
    return SDL_GetTicksNS();
}
