/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include <Common/Expected.h>
#include <Common/Types.h>

namespace {

using Common::Expected;

auto failing_leaf() -> Expected<u32>
{
    return OA_ERROR("Leaf failed with {}", 42);
}

constexpr u32 FAILING_LEAF_LINE = __LINE__ - 3;

auto propagates_value() -> Expected<u32>
{
    u32 value {};
    TRY_ASSIGN(value, failing_leaf());
    return value;
}

auto propagates_void() -> Expected<void>
{
    TRY(failing_leaf());
    return {};
}

auto propagates_twice() -> Expected<u32>
{
    u32 value {};
    TRY_ASSIGN(value, propagates_value());
    return value;
}

}

TEST(Error, OriginIsTheFailingLineNotTheStandardLibrary)
{
    auto const result = failing_leaf();
    ASSERT_FALSE(result.has_value());

    auto const origin = result.error().origin();
    EXPECT_EQ(origin.line(), FAILING_LEAF_LINE);
    EXPECT_NE(std::string_view(origin.file_name()).find("Error.test.cpp"), std::string_view::npos);
}

TEST(Error, FormatsItsMessageArguments)
{
    auto const result = failing_leaf();
    ASSERT_FALSE(result.has_value());

    EXPECT_EQ(result.error().message(), "Leaf failed with 42");
}

TEST(Error, TryAssignPropagatesMessageAndOriginUntouched)
{
    auto const result = propagates_value();
    ASSERT_FALSE(result.has_value());

    EXPECT_EQ(result.error().message(), "Leaf failed with 42");
    EXPECT_EQ(result.error().origin().line(), FAILING_LEAF_LINE);
}

TEST(Error, TryPropagatesFromAVoidResult)
{
    auto const result = propagates_void();
    ASSERT_FALSE(result.has_value());

    EXPECT_EQ(result.error().message(), "Leaf failed with 42");
    EXPECT_EQ(result.error().origin().line(), FAILING_LEAF_LINE);
}

TEST(Error, SurvivesSeveralFramesUnchanged)
{
    auto const result = propagates_twice();
    ASSERT_FALSE(result.has_value());

    EXPECT_EQ(result.error().message(), "Leaf failed with 42");
    EXPECT_EQ(result.error().origin().line(), FAILING_LEAF_LINE);
}

TEST(Error, RendersMessageAndOrigin)
{
    auto const result = failing_leaf();
    ASSERT_FALSE(result.has_value());

    auto const rendered = std::format("{}", result.error());
    EXPECT_TRUE(rendered.starts_with("Leaf failed with 42 (Error.test.cpp:"));
    EXPECT_TRUE(rendered.ends_with(std::format(":{})", FAILING_LEAF_LINE)));
    EXPECT_EQ(rendered, result.error().to_string());
}

TEST(Error, IsStreamableForTestAssertions)
{
    auto const result = failing_leaf();
    ASSERT_FALSE(result.has_value());

    std::ostringstream stream;
    stream << result.error();
    EXPECT_EQ(stream.str(), result.error().to_string());
}

TEST(Error, WorksAsTheErrorHalfOfAVoidResult)
{
    auto fail = []() -> Expected<void> { return OA_ERROR("No value here"); };
    auto succeed = []() -> Expected<void> { return {}; };

    EXPECT_FALSE(fail().has_value());
    EXPECT_EQ(fail().error().message(), "No value here");
    EXPECT_TRUE(succeed().has_value());
}
