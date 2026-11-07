/**
 * @file android_test.cpp
 * @brief End-to-end Android tests consuming the ascend-forge Prefab package.
 */

#include <android/log.h>
#include <ascend_forge/version.hpp>
#include <jni.h>
#include <string>
#include <string_view>

#include "arena_tests.h"

namespace
{

constexpr const char *kLogTag = "ascend-forge-android-test";

class TestRun
{
  public:
    void Check(bool condition, std::string_view message)
    {
        if (condition)
        {
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "PASS: %.*s",
                                static_cast<int>(message.size()), message.data());
            return;
        }
        ++failures_;
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "FAIL: %.*s",
                            static_cast<int>(message.size()), message.data());
    }

    [[nodiscard]] int failures() const noexcept
    {
        return failures_;
    }

  private:
    int failures_ = 0;
};

int RunTests()
{
    TestRun run;
    const std::string_view version = ascend_forge::Version();
    run.Check(!version.empty(), "Prefab package exports the generated version API");
    const std::string major_prefix = std::to_string(ascend_forge::kVersionMajor) + ".";
    run.Check(version.substr(0, major_prefix.size()) == major_prefix,
              "Version() matches the compiled major version");
    run.Check(ascend_forge::kVersionMajor >= 0, "major version is non-negative");
    run.Check(ArenaTests_Run() == 0, "arena checks passed");
    return run.failures();
}

} // namespace

extern "C" JNIEXPORT jint JNICALL
Java_com_example_ascendforge_test_TestActivity_runNativeTests(JNIEnv *, jclass)
{
    return RunTests();
}
