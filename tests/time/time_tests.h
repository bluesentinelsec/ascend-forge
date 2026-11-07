/**
 * @file time_tests.h
 * @brief Entry point for the self-hosted time checks.
 *
 * Platform runners compile time_test.c into their own test binary and call
 * TimeTests_Run. The desktop executable keeps main.
 */

#ifndef ASCEND_FORGE_TIME_TESTS_H
#define ASCEND_FORGE_TIME_TESTS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run every time check.
 *
 * @return The number of failed checks. Zero means every check passed.
 */
int TimeTests_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_TIME_TESTS_H */
