/**
 * @file fork_tests.h
 * @brief Entry point for the self-hosted fork-join checks.
 *
 * Platform runners compile fork_test.c into their own test binary and call
 * ForkJoinTests_Run. The desktop executable keeps main.
 */

#ifndef ASCEND_FORGE_FORK_TESTS_H
#define ASCEND_FORGE_FORK_TESTS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run every fork-join check.
 *
 * @return The number of failed checks. Zero means every check passed.
 */
int ForkJoinTests_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_FORK_TESTS_H */
