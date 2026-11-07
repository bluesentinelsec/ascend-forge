/**
 * @file index.h
 * @brief Half-open index range over a contiguous array.
 *
 * A job owns one [begin, end) range. Split hands each worker a part that no
 * other worker writes. ForEach walks one range on the calling thread. This
 * primitive does not allocate and does not start a thread.
 *
 * The caller owns the input span, the write span, and the arena. The walk
 * passes the index and the caller's data. It does not look at those spans.
 */

#ifndef ASCEND_FORGE_INDEX_H
#define ASCEND_FORGE_INDEX_H

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Half-open index range. end is not included. An empty range has begin == end.
 */
typedef struct IndexRange {
    Uint32 begin;
    Uint32 end;
} IndexRange;

/**
 * Called once per index in [begin, end). data is the pointer the caller
 * passed. NULL data is allowed when the function does not read it.
 */
typedef void (*IndexRangeFn)(Uint32 index, void *data);

/**
 * @brief Store a half-open range.
 *
 * end < begin is rejected and the range is left untouched.
 *
 * @param range Range to store. NULL is rejected.
 * @param begin First index.
 * @param end Index one past the last index.
 * @return false when range is NULL or end is before begin.
 */
bool IndexRange_Init(IndexRange *range, Uint32 begin, Uint32 end);

/**
 * @brief Count the indexes in the range.
 *
 * An empty range counts as zero. A range whose end is before its begin is
 * rejected and count is left untouched.
 *
 * @param count Receives end - begin. On failure this is left untouched.
 * @return false when an argument is NULL or the range is not ordered.
 */
bool IndexRange_Count(const IndexRange *range, Uint32 *count);

/**
 * @brief Report whether index is in [begin, end).
 *
 * The end index is outside the range. An empty range contains nothing.
 *
 * @param contains Receives the answer. On failure this is left untouched.
 * @return false when an argument is NULL or the range is not ordered.
 */
bool IndexRange_Contains(const IndexRange *range, Uint32 index, bool *contains);

/**
 * @brief Call fn once for each index in order.
 *
 * An empty range calls fn zero times and returns true. The walk stays on
 * the calling thread.
 *
 * @param fn Function to call. NULL is rejected.
 * @param data Pointer handed to fn. NULL is allowed.
 * @return false when range or fn is NULL, or the range is not ordered.
 */
bool IndexRange_ForEach(const IndexRange *range, IndexRangeFn fn, void *data);

/**
 * @brief Split range into part_count contiguous parts.
 *
 * The parts cover [begin, end) in order, with no gap and no overlap. The
 * first (count % part_count) parts each receive one extra index. A part past
 * the last index is empty. A part count of zero is rejected and parts is
 * left untouched.
 *
 * @param parts Array of part_count ranges. NULL is rejected.
 * @param part_count How many parts to write. Zero is rejected.
 * @return false when an argument is NULL, part_count is zero, or the range
 *         is not ordered.
 */
bool IndexRange_Split(const IndexRange *range, IndexRange *parts, Uint32 part_count);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_INDEX_H */
