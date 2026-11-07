/**
 * @file atomic_tests.h
 * @brief Entry point for the self-hosted atomic checks.
 *
 * Platform runners compile atomic_test.c into their own test binary and call
 * AtomicTests_Run. The desktop executable keeps main.
 */

#ifndef ASCEND_FORGE_ATOMIC_TESTS_H
#define ASCEND_FORGE_ATOMIC_TESTS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run every atomic check.
 *
 * @return The number of failed checks. Zero means every check passed.
 */
int AtomicTests_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_ATOMIC_TESTS_H */
