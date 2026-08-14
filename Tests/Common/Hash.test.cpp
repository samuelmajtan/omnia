/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <gtest/gtest.h>

#include <Common/Hash.h>

TEST(Hash, KnownVectors)
{
    EXPECT_EQ(Hash::fnv1a(std::string_view("")), 0xCBF29CE484222325ULL);
    EXPECT_EQ(Hash::fnv1a(std::string_view("a")), 0xAF63DC4C8601EC8CULL);
    EXPECT_EQ(Hash::fnv1a(std::string_view("foobar")), 0x85944171F73967E8ULL);
}

TEST(Hash, IsConstexpr)
{
    static_assert(Hash::fnv1a(std::string_view("a")) == 0xAF63DC4C8601EC8CULL);
    SUCCEED();
}

TEST(Hash, DiffersOnSmallChange)
{
    EXPECT_NE(Hash::fnv1a(std::string_view("GeometryPass")), Hash::fnv1a(std::string_view("GeometryPasa")));
}

TEST(Hash, ByteAndTextOverloadsAgree)
{
    std::string_view const text = "vec3 reconstruct_world_pos";
    auto const bytes = std::as_bytes(std::span(text));

    EXPECT_EQ(Hash::fnv1a(text), Hash::fnv1a(bytes));
}

TEST(Hash, SeedChainingIsOrderSensitive)
{
    auto const forward = Hash::fnv1a(std::string_view("second"), Hash::fnv1a(std::string_view("first")));
    auto const backward = Hash::fnv1a(std::string_view("first"), Hash::fnv1a(std::string_view("second")));

    EXPECT_NE(forward, backward);
}

TEST(Hash, ChainingEqualsConcatenation)
{
    auto const chained = Hash::fnv1a(std::string_view("b"), Hash::fnv1a(std::string_view("a")));
    EXPECT_EQ(chained, Hash::fnv1a(std::string_view("ab")));
}
