/**
 * @file json_tests.h
 * @brief Entry point for the self-hosted JSON checks.
 *
 * Platform runners compile json_test.c into their own test binary and call
 * JsonTests_Run. The desktop executable keeps main.
 */

#ifndef ASCEND_FORGE_JSON_TESTS_H
#define ASCEND_FORGE_JSON_TESTS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run every JSON check.
 *
 * @return The number of failed checks. Zero means every check passed.
 */
int JsonTests_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_JSON_TESTS_H */
