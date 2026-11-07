/**
 * @file android_test.c
 * @brief End-to-end Android tests consuming the ascend-forge Prefab package.
 */

#include <android/log.h>
#include <ascend_forge/version.h>
#include <jni.h>

#include <stdio.h>
#include <string.h>

#include "arena_tests.h"
#include "utf8_tests.h"
#include "result_tests.h"
#include "io_tests.h"
#include "json_tests.h"
#include "slice_tests.h"
#include "atomic_tests.h"
#include "thread_tests.h"
#include "time_tests.h"
#include "index_tests.h"
#include "fork_tests.h"
#include "cli_tests.h"

static const char *kLogTag = "ascend-forge-android-test";
static int g_failures = 0;

static void check(int condition, const char *message)
{
    if (condition) {
        __android_log_print(ANDROID_LOG_INFO, kLogTag, "PASS: %s", message);
        return;
    }
    g_failures += 1;
    __android_log_print(ANDROID_LOG_ERROR, kLogTag, "FAIL: %s", message);
}

static int runTests(void)
{
    char major_prefix[32];
    const char *version = Version_String();
    const int written = snprintf(major_prefix, sizeof major_prefix, "%d.",
                                 ASCEND_FORGE_VERSION_MAJOR);

    g_failures = 0;
    check(version != NULL && version[0] != '\0',
          "Prefab package exports the generated version API");
    check(written > 0 && version != NULL &&
              strncmp(version, major_prefix, (size_t)written) == 0,
          "Version_String matches the compiled major version");
    check(ArenaTests_Run() == 0, "arena checks passed");
    check(Utf8Tests_Run() == 0, "utf8 checks passed");
    check(ResultTests_Run() == 0, "result checks passed");
    check(IoTests_Run() == 0, "io checks passed");
    check(JsonTests_Run() == 0, "json checks passed");
    check(SliceTests_Run() == 0, "slice checks passed");
    check(AtomicTests_Run() == 0, "atomic checks passed");
    check(ThreadTests_Run() == 0, "thread checks passed");
    check(TimeTests_Run() == 0, "time checks passed");
    check(IndexRangeTests_Run() == 0, "index checks passed");
    check(ForkJoinTests_Run() == 0, "fork checks passed");
    check(CliTests_Run() == 0, "cli checks passed");
    return g_failures;
}

JNIEXPORT jint JNICALL
Java_com_example_ascendforge_test_TestActivity_runNativeTests(JNIEnv *env, jclass clazz)
{
    (void)env;
    (void)clazz;
    return runTests();
}
