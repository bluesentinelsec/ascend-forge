/**
 * @file utf8_tests.h
 * @brief Entry point for the self-hosted UTF-8 checks.
 *
 * Platform runners compile utf8_test.c into their own test binary and call
 * Utf8Tests_Run. The desktop executable keeps main.
 */

#ifndef ASCEND_FORGE_UTF8_TESTS_H
#define ASCEND_FORGE_UTF8_TESTS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run every byte-span and UTF-8 check.
 *
 * @return The number of failed checks. Zero means every check passed.
 */
int Utf8Tests_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_UTF8_TESTS_H */
