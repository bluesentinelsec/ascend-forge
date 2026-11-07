/**
 * @file arena_tests.h
 * @brief Entry point for the self-hosted arena checks.
 *
 * Platform runners compile arena_test.c into their own test binary and call
 * ArenaTests_Run. The desktop executable keeps main.
 */

#ifndef ASCEND_FORGE_ARENA_TESTS_H
#define ASCEND_FORGE_ARENA_TESTS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run every arena check.
 *
 * @return The number of failed checks. Zero means every check passed.
 */
int ArenaTests_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_ARENA_TESTS_H */
