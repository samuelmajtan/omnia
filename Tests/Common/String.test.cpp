/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <gtest/gtest.h>

#include <Common/String.h>

TEST(StringTest, TrimsBothEnds)
{
    EXPECT_EQ(String::trimmed("  key  "), "key");
    EXPECT_EQ(String::trimmed("\t\n key = value \r\n"), "key = value");
}

TEST(StringTest, TrimsOnlyTheRequestedEnd)
{
    EXPECT_EQ(String::trimmed_left("  key  "), "key  ");
    EXPECT_EQ(String::trimmed_right("  key  "), "  key");
}

TEST(StringTest, LeavesInteriorWhitespaceAlone)
{
    EXPECT_EQ(String::trimmed("  a \t b  "), "a \t b");
}

TEST(StringTest, HandlesEmptyAndAllWhitespace)
{
    EXPECT_EQ(String::trimmed(""), "");
    EXPECT_EQ(String::trimmed(" \t\n\v\f\r"), "");
    EXPECT_TRUE(String::trimmed(" \t ").empty());
}

TEST(StringTest, LeavesUntrimmedTextUntouched)
{
    EXPECT_EQ(String::trimmed("key"), "key");
}

TEST(StringTest, IsConstexpr)
{
    static_assert(String::trimmed("  key  ") == "key");
    static_assert(String::trimmed_left("\tkey") == "key");
    static_assert(String::trimmed_right("key\n") == "key");
    static_assert(String::trimmed("   ").empty());
    static_assert(String::is_whitespace(' '));
    static_assert(!String::is_whitespace('k'));
    static_assert(!String::is_whitespace('\0'));
}
