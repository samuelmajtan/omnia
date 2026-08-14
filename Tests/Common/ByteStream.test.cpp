/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <gtest/gtest.h>

#include <Common/ByteStream.h>

namespace {

struct Foo {
    u32 a;
    f32 b;

    auto operator==(Foo const& other) const -> bool = default;
};

auto reader_over(Binary::ByteWriter const& writer) -> Binary::ByteReader
{
    return Binary::ByteReader(writer.bytes());
}

}

TEST(ByteStream, ScalarRoundTrip)
{
    Binary::ByteWriter writer;
    writer.write<u32>(0xDEADBEEF);
    writer.write<f32>(1.5F);
    writer.write<u8>(7);

    auto reader = reader_over(writer);
    EXPECT_EQ(reader.read<u32>().value(), 0xDEADBEEF);
    EXPECT_EQ(reader.read<f32>().value(), 1.5F);
    EXPECT_EQ(reader.read<u8>().value(), 7);
    EXPECT_TRUE(reader.is_exhausted());
}

TEST(ByteStream, StructRoundTrip)
{
    Foo const value { .a = 42, .b = -3.25F };

    Binary::ByteWriter writer;
    writer.write(value);

    auto reader = reader_over(writer);
    EXPECT_EQ(reader.read<Foo>().value(), value);
}

TEST(ByteStream, VectorRoundTrip)
{
    std::vector<Foo> const values { { 1, 1.0F }, { 2, 2.0F }, { 3, 3.0F } };

    Binary::ByteWriter writer;
    writer.write_vector(values);

    auto reader = reader_over(writer);
    EXPECT_EQ(reader.read_vector<Foo>().value(), values);
    EXPECT_TRUE(reader.is_exhausted());
}

TEST(ByteStream, EmptyVectorRoundTrip)
{
    Binary::ByteWriter writer;
    writer.write_vector(std::vector<Foo> {});

    auto reader = reader_over(writer);
    auto const values = reader.read_vector<Foo>();
    ASSERT_TRUE(values.has_value());
    EXPECT_TRUE(values.value().empty());
    EXPECT_TRUE(reader.is_exhausted());
}

TEST(ByteStream, StringRoundTrip)
{
    Binary::ByteWriter writer;
    writer.write_string("Models/sponza/Sponza");
    writer.write_string("");

    auto reader = reader_over(writer);
    EXPECT_EQ(reader.read_string().value(), "Models/sponza/Sponza");
    EXPECT_EQ(reader.read_string().value(), "");
    EXPECT_TRUE(reader.is_exhausted());
}

TEST(ByteStream, OptionalRoundTrip)
{
    Binary::ByteWriter writer;
    writer.write_optional(std::optional<u32> { 99 });
    writer.write_optional(std::optional<u32> {});

    auto reader = reader_over(writer);
    EXPECT_EQ(reader.read_optional<u32>().value(), std::optional<u32> { 99 });
    EXPECT_EQ(reader.read_optional<u32>().value(), std::nullopt);
    EXPECT_TRUE(reader.is_exhausted());
}

TEST(ByteStream, InterleavedRoundTripPreservesOrder)
{
    Binary::ByteWriter writer;
    writer.write<u32>(1);
    writer.write_string("middle");
    writer.write_vector(std::vector<u32> { 4, 5, 6 });
    writer.write<u8>(2);

    auto reader = reader_over(writer);
    EXPECT_EQ(reader.read<u32>().value(), 1U);
    EXPECT_EQ(reader.read_string().value(), "middle");
    EXPECT_EQ(reader.read_vector<u32>().value(), (std::vector<u32> { 4, 5, 6 }));
    EXPECT_EQ(reader.read<u8>().value(), 2);
    EXPECT_TRUE(reader.is_exhausted());
}

TEST(ByteStream, ReadFromEmptyStreamFails)
{
    Binary::ByteReader reader({});
    EXPECT_FALSE(reader.read<u32>().has_value());
}

TEST(ByteStream, TruncatedScalarFails)
{
    Binary::ByteWriter writer;
    writer.write<u32>(1);

    auto const truncated = writer.bytes().first(2);
    Binary::ByteReader reader(truncated);
    EXPECT_FALSE(reader.read<u32>().has_value());
}

TEST(ByteStream, TruncatedVectorPayloadFails)
{
    Binary::ByteWriter writer;
    writer.write_vector(std::vector<u32> { 1, 2, 3, 4 });

    auto const truncated = writer.bytes().first(sizeof(u64) + sizeof(u32));
    Binary::ByteReader reader(truncated);
    EXPECT_FALSE(reader.read_vector<u32>().has_value());
}

TEST(ByteStream, TruncatedOptionalPayloadFails)
{
    Binary::ByteWriter writer;
    writer.write_optional(std::optional<u32> { 5 });

    auto const truncated = writer.bytes().first(1);
    Binary::ByteReader reader(truncated);
    EXPECT_FALSE(reader.read_optional<u32>().has_value());
}

TEST(ByteStream, ReadPastEndAfterValidReadsFails)
{
    Binary::ByteWriter writer;
    writer.write<u32>(1);

    auto reader = reader_over(writer);
    EXPECT_TRUE(reader.read<u32>().has_value());
    EXPECT_TRUE(reader.is_exhausted());
    EXPECT_FALSE(reader.read<u8>().has_value());
}
