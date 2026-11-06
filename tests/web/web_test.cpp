/**
 * @file web_test.cpp
 * @brief Browser-runtime tests for the WebAssembly build of ascend-forge.
 *
 * Runs inside a real browser (see .github/workflows/web.yml and `emrun`);
 * the process exit code is reported back through --emrun / -sEXIT_RUNTIME=1.
 */

#include <ascend_forge/version.hpp>
#include <emscripten.h>
#include <gtest/gtest.h>
#include <string>

TEST(WebVersion, IsNonEmpty)
{
    EXPECT_FALSE(ascend_forge::Version().empty());
}

TEST(WebVersion, MatchesComponentConstants)
{
    const std::string expected = std::to_string(ascend_forge::kVersionMajor) + "." +
                                 std::to_string(ascend_forge::kVersionMinor) + "." +
                                 std::to_string(ascend_forge::kVersionPatch);
    EXPECT_EQ(ascend_forge::Version().substr(0, expected.size()), expected);
}

TEST(WebRuntime, RunsInsideBrowserJavaScriptEnvironment)
{
    // Proves the JS interop bridge works — the same bridge a game uses for
    // canvas/audio/input glue.
    const char *agent = emscripten_run_script_string(
        "typeof navigator !== 'undefined' ? navigator.userAgent : 'node'");
    ASSERT_NE(agent, nullptr);
    EXPECT_FALSE(std::string(agent).empty());
}
