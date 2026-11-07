/**
 * @file fork.h
 * @brief Fork-join over one index range.
 *
 * The caller chooses how many workers to start and provides their storage.
 * Each worker walks one part from IndexRange_Split. The call returns when
 * the completion count is zero.
 *
 * Workers do not call SDL window, event, or renderer functions. This
 * primitive does not call them either. On web, where threads do not start,
 * the same call walks each part on the caller.
 */

#ifndef ASCEND_FORGE_FORK_H
#define ASCEND_FORGE_FORK_H

#include "ascend_forge/index.h"
#include "ascend_forge/thread.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run one index-range job on worker_count workers.
 *
 * workers must be zeroed. The call inits them. A worker count of zero, a
 * null argument, or a range whose end is before its begin is rejected and
 * the function is not called.
 *
 * The temporary worker records are one SDL allocation freed before return.
 * When that allocation fails, the call logs and exits.
 *
 * @param workers Array of worker_count threads. NULL is rejected.
 * @param worker_count How many workers to start. Zero is rejected.
 * @param range Parent range to split. NULL is rejected.
 * @param fn Called once per index. NULL is rejected.
 * @param data Pointer handed to fn. NULL is allowed.
 * @return false when the job cannot run. A failed start can leave earlier
 *         parts written.
 */
bool ForkJoin_Run(Thread *workers, Uint32 worker_count, const IndexRange *range, IndexRangeFn fn,
                   void *data);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_FORK_H */
