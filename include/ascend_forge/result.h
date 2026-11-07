/**
 * @file result.h
 * @brief One result shape for every fallible call.
 *
 * A result says whether the call worked, a code, what was expected, what
 * arrived, and what to do next. The strings borrow static text or memory in
 * the caller's arena. A result does not allocate. Out of memory stays fatal
 * inside the arena and is not a result.
 *
 * Code zero is success. Any other code belongs to the caller. A callee that
 * documents the contract leaves its output untouched when it returns a
 * failure. Jobs may read a result. The borrowed strings stay valid for as
 * long as the memory they point at.
 */

#ifndef ASCEND_FORGE_RESULT_H
#define ASCEND_FORGE_RESULT_H

#include "ascend_forge/utf8.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Fallible-call result. The three strings are never NULL. */
typedef struct Result {
    bool ok;
    Uint32 code;
    Utf8 expected;
    Utf8 actual;
    Utf8 next;
} Result;

/**
 * @brief Success.
 *
 * The code is zero. Each string is empty and points at a real address.
 * This call does not allocate.
 */
Result Result_Ok(void);

/**
 * @brief Write a failure that borrows expected, actual, and next.
 *
 * This call does not allocate. Static text and text already in the caller's
 * arena are both valid. An empty string is allowed and still has a real
 * pointer.
 *
 * @param code Caller-defined failure code. Zero is rejected.
 * @param expected What the call required.
 * @param actual What arrived.
 * @param next What would make the call valid.
 * @param out Receives the failure. On rejection this is left untouched.
 * @return false when out is NULL, code is zero, or any string pointer is
 *         NULL.
 */
bool Result_Fail(Uint32 code, Utf8 expected, Utf8 actual, Utf8 next, Result *out);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_RESULT_H */
