/**
 * @file index_tests.h
 * @brief Entry point for the self-hosted index-range checks.
 *
 * Platform runners compile index_test.c into their own test binary and call
 * IndexRangeTests_Run. The desktop executable keeps main.
 */

#ifndef ASCEND_FORGE_INDEX_TESTS_H
#define ASCEND_FORGE_INDEX_TESTS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run every index-range check.
 *
 * @return The number of failed checks. Zero means every check passed.
 */
int IndexRangeTests_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_INDEX_TESTS_H */
