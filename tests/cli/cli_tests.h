/**
 * @file cli_tests.h
 * @brief Entry point for the self-hosted command-line checks.
 *
 * Platform runners compile cli_test.c into their own test binary and call
 * CliTests_Run. The desktop executable keeps main.
 */

#ifndef ASCEND_FORGE_CLI_TESTS_H
#define ASCEND_FORGE_CLI_TESTS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run every command-line check.
 *
 * @return The number of failed checks. Zero means every check passed.
 */
int CliTests_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_CLI_TESTS_H */
