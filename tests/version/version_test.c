/**
 * @file version_test.c
 * @brief Self-hosted checks for the package version.
 */

#include "ascend_forge/version.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static int g_failed = 0;

static void expect_true(bool condition, const char *name)
{
    if (condition) {
        return;
    }
    fprintf(stderr, "FAIL: %s\n", name);
    g_failed += 1;
}

static void test_version_is_non_empty(void)
{
    const char *version = Version_String();

    expect_true(version != NULL && version[0] != '\0', "Version_NonEmpty");
}

static void test_version_matches_the_components(void)
{
    char expected[64];
    const char *version = Version_String();
    const int written = snprintf(expected, sizeof expected, "%d.%d.%d",
                                 ASCEND_FORGE_VERSION_MAJOR,
                                 ASCEND_FORGE_VERSION_MINOR,
                                 ASCEND_FORGE_VERSION_PATCH);

    expect_true(written > 0 && (size_t)written < sizeof expected, "Version_Format");
    expect_true(strcmp(version, expected) == 0, "Version_Matches");
}

static void test_version_has_three_numeric_components(void)
{
    const char *version = Version_String();
    const char *first = strchr(version, '.');
    const char *second = NULL;

    expect_true(first != NULL, "Version_FirstDot");
    if (first == NULL) {
        return;
    }
    second = strchr(first + 1, '.');
    expect_true(second != NULL, "Version_SecondDot");
}

int main(void)
{
    test_version_is_non_empty();
    test_version_matches_the_components();
    test_version_has_three_numeric_components();
    if (g_failed != 0) {
        fprintf(stderr, "%d version checks failed\n", g_failed);
        return 1;
    }
    printf("version checks passed\n");
    return 0;
}
