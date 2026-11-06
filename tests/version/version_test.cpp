/**
 * @file version_test.cpp
 * @brief Unit tests for ascend_forge::Version.
 */

#include "ascend_forge/version.hpp"

#include <gtest/gtest.h>
#include <string>
#include <string_view>

namespace
{

TEST(VersionTest, IsNonEmpty)
{
    const std::string_view version = ascend_forge::Version();
    EXPECT_FALSE(version.empty());
}

TEST(VersionTest, MatchesComponentConstants)
{
    // Version() is generated from the root VERSION file; constants must agree.
    const std::string expected = std::to_string(ascend_forge::kVersionMajor) + "." +
                                 std::to_string(ascend_forge::kVersionMinor) + "." +
                                 std::to_string(ascend_forge::kVersionPatch);
    EXPECT_EQ(std::string_view{ascend_forge::Version()}, expected);
    EXPECT_GE(ascend_forge::kVersionMajor, 0);
    EXPECT_GE(ascend_forge::kVersionMinor, 0);
    EXPECT_GE(ascend_forge::kVersionPatch, 0);
}

TEST(VersionTest, HasThreeNumericComponents)
{
    const std::string_view version = ascend_forge::Version();
    EXPECT_NE(version.find('.'), std::string_view::npos);
    EXPECT_NE(version.rfind('.'), std::string_view::npos);
    EXPECT_NE(version.find('.'), version.rfind('.'));
}

} // namespace
