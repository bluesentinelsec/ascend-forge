/**
 * @file thread_tests.h
 * @brief Entry point for the self-hosted thread checks.
 *
 * Platform runners compile thread_test.c into their own test binary and call
 * ThreadTests_Run. The desktop executable keeps main.
 */

#ifndef ASCEND_FORGE_THREAD_TESTS_H
#define ASCEND_FORGE_THREAD_TESTS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run every thread check.
 *
 * @return The number of failed checks. Zero means every check passed.
 */
int ThreadTests_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_THREAD_TESTS_H */
