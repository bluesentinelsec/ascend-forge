/**
 * @file result_tests.h
 * @brief Entry point for the self-hosted result checks.
 *
 * Platform runners compile result_test.c into their own test binary and call
 * ResultTests_Run. The desktop executable keeps main.
 */

#ifndef ASCEND_FORGE_RESULT_TESTS_H
#define ASCEND_FORGE_RESULT_TESTS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run every result check.
 *
 * @return The number of failed checks. Zero means every check passed.
 */
int ResultTests_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_RESULT_TESTS_H */
