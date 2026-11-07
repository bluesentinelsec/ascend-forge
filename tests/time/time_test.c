/**
 * @file time_test.c
 * @brief Self-hosted checks for the time primitive.
 */

#include "ascend_forge/time.h"

#include "time_tests.h"

#include <stdio.h>

static int g_failed = 0;

static void expect_true(bool condition, const char *name)
{
    if (condition) {
        return;
    }
    fprintf(stderr, "FAIL: %s\n", name);
    g_failed += 1;
}

static void test_second_reading_is_not_earlier(void)
{
    const Uint64 first = Time_Now();
    const Uint64 second = Time_Now();

    expect_true(second >= first, "Time_Ordered");
}

int TimeTests_Run(void)
{
    g_failed = 0;
    test_second_reading_is_not_earlier();
    return g_failed;
}

#ifndef ASCEND_FORGE_TIME_TESTS_NO_MAIN
int main(void)
{
    const int failed = TimeTests_Run();

    if (failed != 0) {
        fprintf(stderr, "%d time checks failed\n", failed);
        return 1;
    }
    printf("time checks passed\n");
    return 0;
}
#endif
