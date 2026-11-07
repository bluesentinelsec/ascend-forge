/**
 * @file io_tests.h
 * @brief Entry point for the self-hosted IO checks.
 *
 * Platform runners compile io_test.c into their own test binary and call
 * IoTests_Run. The desktop executable keeps main.
 */

#ifndef ASCEND_FORGE_IO_TESTS_H
#define ASCEND_FORGE_IO_TESTS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run every file and path check.
 *
 * @return The number of failed checks. Zero means every check passed.
 */
int IoTests_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_IO_TESTS_H */
