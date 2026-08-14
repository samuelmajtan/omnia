/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <cmath>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include <Common/Random.h>
#include <Common/UUID.h>

using Common::Random;

namespace {

auto next_values(Random& random, std::size_t count) -> std::vector<u64>
{
    std::vector<u64> values;
    values.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        values.push_back(random.next_u64());
    }
    return values;
}

}

TEST(RandomTest, SameSeedReplaysTheSameStream)
{
    Random first(1234);
    Random second(1234);

    EXPECT_EQ(next_values(first, 64), next_values(second, 64));
}

TEST(RandomTest, DifferentSeedsDiverge)
{
    Random first(1234);
    Random second(1235);

    EXPECT_NE(next_values(first, 64), next_values(second, 64));
}

TEST(RandomTest, ReseedRestartsTheStream)
{
    Random random(7);
    auto const expected = next_values(random, 32);

    random.reseed(7);
    EXPECT_EQ(next_values(random, 32), expected);
}

TEST(RandomTest, EntropySeededInstancesDiffer)
{
    Random first;
    Random second;

    EXPECT_NE(next_values(first, 16), next_values(second, 16));
}

TEST(RandomTest, EntropyReseedChangesTheStream)
{
    Random random(42);
    auto const seeded = next_values(random, 16);

    random.reseed();
    EXPECT_NE(next_values(random, 16), seeded);
}

TEST(RandomTest, SharedIsOneInstancePerThread)
{
    EXPECT_EQ(&Random::shared(), &Random::shared());
    EXPECT_NE(Random::shared().next_u64(), Random::shared().next_u64());
}

TEST(RandomTest, UnitFloatsStayInRange)
{
    Random random(99);
    for (auto i = 0; i < 10000; ++i) {
        auto const value32 = random.next_f32();
        ASSERT_GE(value32, 0.0f);
        ASSERT_LT(value32, 1.0f);

        auto const value64 = random.next_f64();
        ASSERT_GE(value64, 0.0);
        ASSERT_LT(value64, 1.0);
    }
}

TEST(RandomTest, IntegerRangeIsInclusiveAndCoversBothEnds)
{
    Random random(5);
    std::set<i32> seen;
    for (auto i = 0; i < 10000; ++i) {
        auto const value = random.range(-3, 4);
        ASSERT_GE(value, -3);
        ASSERT_LE(value, 4);
        seen.insert(value);
    }
    EXPECT_EQ(seen.size(), 8u);
}

TEST(RandomTest, IntegerRangeOfOneValue)
{
    Random random(5);
    for (auto i = 0; i < 100; ++i) {
        EXPECT_EQ(random.range(9, 9), 9);
    }
}

TEST(RandomTest, IntegerRangeSpansTheWholeWidth)
{
    Random random(5);
    auto low = false;
    auto high = false;
    for (auto i = 0; i < 1000; ++i) {
        auto const value = random.range<u64>(0, std::numeric_limits<u64>::max());
        low = low || value < (std::numeric_limits<u64>::max() / 4);
        high = high || value > (std::numeric_limits<u64>::max() / 4 * 3);
    }
    EXPECT_TRUE(low);
    EXPECT_TRUE(high);
}

TEST(RandomTest, BoolProducesBothOutcomes)
{
    Random random(3);
    auto trues = 0;
    for (auto i = 0; i < 1000; ++i) {
        trues += random.next_bool() ? 1 : 0;
    }
    EXPECT_GT(trues, 0);
    EXPECT_LT(trues, 1000);
}

TEST(RandomTest, DrivesReproducibleUUIDs)
{
    Random first(2026);
    Random second(2026);

    auto const uuid = Common::UUID::generate(first);
    EXPECT_EQ(uuid, Common::UUID::generate(second));
    EXPECT_EQ(uuid.version(), 4);
    EXPECT_NE(uuid, Common::UUID::generate(first));
}
