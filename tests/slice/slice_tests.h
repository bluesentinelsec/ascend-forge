/**
 * @file slice_tests.h
 * @brief Entry point for the self-hosted slice checks.
 *
 * Platform runners compile slice_test.c into their own test binary and call
 * SliceTests_Run. The desktop executable keeps main.
 */

#ifndef ASCEND_FORGE_SLICE_TESTS_H
#define ASCEND_FORGE_SLICE_TESTS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run every slice check.
 *
 * @return The number of failed checks. Zero means every check passed.
 */
int SliceTests_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_SLICE_TESTS_H */
