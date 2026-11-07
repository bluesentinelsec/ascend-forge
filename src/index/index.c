/**
 * @file index.c
 * @brief Half-open index range. No allocation and no thread.
 */

#include "ascend_forge/index.h"

bool IndexRange_Init(IndexRange *range, Uint32 begin, Uint32 end)
{
    if (range == NULL) {
        SDL_Log("IndexRange_Init expected a range, received NULL");
        return false;
    }
    if (end < begin) {
        SDL_Log("IndexRange_Init expected an end at or after begin %" SDL_PRIu32
                ", received %" SDL_PRIu32,
                begin, end);
        return false;
    }
    range->begin = begin;
    range->end = end;
    return true;
}

bool IndexRange_Count(const IndexRange *range, Uint32 *count)
{
    if (range == NULL || count == NULL) {
        SDL_Log("IndexRange_Count expected a range and a count");
        return false;
    }
    if (range->end < range->begin) {
        SDL_Log("IndexRange_Count expected an end at or after begin %" SDL_PRIu32
                ", received %" SDL_PRIu32,
                range->begin, range->end);
        return false;
    }
    *count = range->end - range->begin;
    return true;
}

bool IndexRange_Contains(const IndexRange *range, Uint32 index, bool *contains)
{
    if (range == NULL || contains == NULL) {
        SDL_Log("IndexRange_Contains expected a range and a result");
        return false;
    }
    if (range->end < range->begin) {
        SDL_Log("IndexRange_Contains expected an end at or after begin %" SDL_PRIu32
                ", received %" SDL_PRIu32,
                range->begin, range->end);
        return false;
    }
    *contains = (index >= range->begin) && (index < range->end);
    return true;
}

bool IndexRange_ForEach(const IndexRange *range, IndexRangeFn fn, void *data)
{
    Uint32 index = 0;

    if (range == NULL) {
        SDL_Log("IndexRange_ForEach expected a range, received NULL");
        return false;
    }
    if (fn == NULL) {
        SDL_Log("IndexRange_ForEach expected a function, received NULL");
        return false;
    }
    if (range->end < range->begin) {
        SDL_Log("IndexRange_ForEach expected an end at or after begin %" SDL_PRIu32
                ", received %" SDL_PRIu32,
                range->begin, range->end);
        return false;
    }
    /* Compare against end. The last index is end - 1, so the next index is
     * end and the counter does not wrap. */
    index = range->begin;
    while (index != range->end) {
        fn(index, data);
        index += 1u;
    }
    return true;
}

bool IndexRange_Split(const IndexRange *range, IndexRange *parts, Uint32 part_count)
{
    Uint32 count = 0;
    Uint32 base = 0;
    Uint32 extra = 0;
    Uint32 cursor = 0;
    Uint32 index = 0;

    if (range == NULL || parts == NULL || part_count == 0u) {
        SDL_Log("IndexRange_Split expected a range, a parts array, and a part count "
                "greater than zero");
        return false;
    }
    if (range->end < range->begin) {
        SDL_Log("IndexRange_Split expected an end at or after begin %" SDL_PRIu32
                ", received %" SDL_PRIu32,
                range->begin, range->end);
        return false;
    }
    count = range->end - range->begin;
    base = count / part_count;
    extra = count % part_count;
    cursor = range->begin;
    for (index = 0; index < part_count; index += 1u) {
        const Uint32 size = base + ((index < extra) ? 1u : 0u);

        parts[index].begin = cursor;
        parts[index].end = cursor + size;
        cursor += size;
    }
    return true;
}
