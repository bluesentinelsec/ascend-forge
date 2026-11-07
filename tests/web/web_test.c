/**
 * @file web_test.c
 * @brief Browser-runtime checks for the WebAssembly build.
 *
 * Runs inside a real browser (emrun). The process exit code comes back
 * through --emrun and -sEXIT_RUNTIME=1.
 */

#include "ascend_forge/version.h"

#include <emscripten.h>

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

    expect_true(version != NULL && version[0] != '\0', "Web_Version_NonEmpty");
}

static void test_version_matches_the_components(void)
{
    char expected[64];
    const char *version = Version_String();
    const int written = snprintf(expected, sizeof expected, "%d.%d.%d",
                                 ASCEND_FORGE_VERSION_MAJOR,
                                 ASCEND_FORGE_VERSION_MINOR,
                                 ASCEND_FORGE_VERSION_PATCH);

    expect_true(written > 0, "Web_Version_Format");
    expect_true(version != NULL && strncmp(version, expected, (size_t)written) == 0,
                "Web_Version_Components");
}

static void test_runs_inside_the_browser(void)
{
    const char *agent = emscripten_run_script_string(
        "typeof navigator !== 'undefined' ? navigator.userAgent : 'node'");

    expect_true(agent != NULL && agent[0] != '\0', "Web_Runtime_Agent");
}

int main(void)
{
    test_version_is_non_empty();
    test_version_matches_the_components();
    test_runs_inside_the_browser();
    if (g_failed != 0) {
        fprintf(stderr, "%d web checks failed\n", g_failed);
        return 1;
    }
    printf("web checks passed\n");
    return 0;
}
