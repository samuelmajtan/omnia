/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <format>
#include <unordered_set>

#include <gtest/gtest.h>

#include <Common/ByteStream.h>
#include <Common/UUID.h>

using Common::UUID;

TEST(UUIDTest, DefaultConstructedIsNil)
{
    constexpr UUID uuid;

    EXPECT_EQ(uuid.to_string(), "00000000-0000-0000-0000-000000000000");
    EXPECT_EQ(uuid, UUID());
}

TEST(UUIDTest, GenerateSetsVersionAndVariant)
{
    auto const uuid = UUID::generate();
    auto const bytes = uuid.bytes();

    EXPECT_EQ(uuid.version(), 4);
    EXPECT_EQ(static_cast<u8>(bytes[8]) & 0xC0, 0x80);
}

TEST(UUIDTest, GenerateDoesNotRepeat)
{
    std::unordered_set<UUID> seen;
    for (auto i = 0; i < 10000; ++i) {
        EXPECT_TRUE(seen.insert(UUID::generate()).second);
    }
    EXPECT_EQ(seen.size(), 10000u);
}

TEST(UUIDTest, GenerateNeverCollidesWithNumericSentinels)
{
    for (auto i = 0; i < 1000; ++i) {
        auto const uuid = UUID::generate();
        for (u64 sentinel = 0; sentinel < 8; ++sentinel) {
            EXPECT_FALSE(uuid == UUID(sentinel));
        }
    }
}

TEST(UUIDTest, NumericConstructorIsConstexpr)
{
    constexpr UUID one(1);
    constexpr UUID two(2);

    static_assert(one == UUID(1));
    static_assert(!(one == two));
    EXPECT_NE(one.hash(), two.hash());
}

TEST(UUIDTest, RoundTripsThroughString)
{
    for (auto i = 0; i < 100; ++i) {
        auto const uuid = UUID::generate();
        auto const parsed = UUID::from_string(uuid.to_string());

        ASSERT_TRUE(parsed.has_value());
        EXPECT_EQ(parsed.value(), uuid);
    }
}

TEST(UUIDTest, ToStringIsCanonicalLowercase)
{
    auto const parsed = UUID::from_string("A1B2C3D4-E5F6-4A8B-9C0D-1E2F3A4B5C6D");

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed.value().to_string(), "a1b2c3d4-e5f6-4a8b-9c0d-1e2f3a4b5c6d");
}

TEST(UUIDTest, FromStringUsesCanonicalByteOrder)
{
    auto const parsed = UUID::from_string("00112233-4455-6677-8899-aabbccddeeff");

    ASSERT_TRUE(parsed.has_value());
    auto const bytes = parsed.value().bytes();
    for (std::size_t i = 0; i < UUID::SIZE; ++i) {
        EXPECT_EQ(static_cast<u8>(bytes[i]), static_cast<u8>(i * 0x11)) << "byte " << i;
    }
}

TEST(UUIDTest, FromStringIsConstexpr)
{
    constexpr auto parsed = UUID::from_string("a1b2c3d4-e5f6-4a8b-9c0d-1e2f3a4b5c6d");

    static_assert(parsed.has_value());
    static_assert(parsed.value().version() == 4);
}

TEST(UUIDTest, FromStringRejectsMalformedInput)
{
    EXPECT_FALSE(UUID::from_string("").has_value());
    EXPECT_FALSE(UUID::from_string("not-a-uuid").has_value());
    // One character short.
    EXPECT_FALSE(UUID::from_string("a1b2c3d4-e5f6-4a8b-9c0d-1e2f3a4b5c6").has_value());
    // One character too long.
    EXPECT_FALSE(UUID::from_string("a1b2c3d4-e5f6-4a8b-9c0d-1e2f3a4b5c6dd").has_value());
    // Braced form is not canonical.
    EXPECT_FALSE(UUID::from_string("{a1b2c3d4-e5f6-4a8b-9c0d-1e2f3a4b5c6d}").has_value());
    // Separators in the wrong places.
    EXPECT_FALSE(UUID::from_string("a1b2c3d4e5f6-4a8b-9c0d-1e2f3a4b5c6d--").has_value());
    // A non-hex digit.
    EXPECT_FALSE(UUID::from_string("a1b2c3d4-e5f6-4a8b-9c0d-1e2f3a4b5c6g").has_value());
}

TEST(UUIDTest, EqualIdsHashEqually)
{
    auto const uuid = UUID::generate();
    auto const copy = UUID::from_string(uuid.to_string());

    ASSERT_TRUE(copy.has_value());
    EXPECT_EQ(uuid.hash(), copy.value().hash());
    EXPECT_EQ(std::hash<UUID>()(uuid), std::hash<UUID>()(copy.value()));
}

TEST(UUIDTest, UsableAsUnorderedMapKey)
{
    std::unordered_map<UUID, int> map;
    auto const first = UUID::generate();
    auto const second = UUID::generate();

    map[first] = 1;
    map[second] = 2;

    EXPECT_EQ(map.at(first), 1);
    EXPECT_EQ(map.at(second), 2);
    EXPECT_EQ(map.find(UUID::generate()), map.end());
}

TEST(UUIDTest, FormatsThroughStdFormat)
{
    auto const uuid = UUID::generate();

    EXPECT_EQ(std::format("{}", uuid), uuid.to_string());
    EXPECT_EQ(std::format("[{:>38}]", UUID()), "[  00000000-0000-0000-0000-000000000000]");
}

TEST(UUIDTest, SurvivesBinarySerialization)
{
    auto const uuid = UUID::generate();

    Binary::ByteWriter writer;
    writer.write(uuid);
    EXPECT_EQ(writer.size(), UUID::SIZE);

    Binary::ByteReader reader(writer.bytes());
    auto const read = reader.read<UUID>();

    ASSERT_TRUE(read.has_value());
    EXPECT_EQ(read.value(), uuid);
    EXPECT_TRUE(reader.is_exhausted());
}
