/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <gtest/gtest.h>

#include <Common/ByteStream.h>
#include <Common/File.h>

namespace {

class FileWrite : public testing::Test {
protected:
    void SetUp() override
    {
        std::error_code error;
        m_directory = std::filesystem::path(testing::TempDir()) / "OmniaFileTest";
        std::filesystem::remove_all(m_directory, error);
        std::filesystem::create_directories(m_directory, error);
        ASSERT_FALSE(error) << error.message();
    }

    void TearDown() override
    {
        std::error_code error;
        std::filesystem::remove_all(m_directory, error);
    }

    auto path_for(std::string_view name) const -> std::filesystem::path
    {
        return m_directory / name;
    }

    std::filesystem::path m_directory;
};

}

TEST_F(FileWrite, TextRoundTrip)
{
    auto const path = path_for("sidecar.omnia");
    auto const content = std::string("uuid = 3f2504e0-4f89-11d3-9a0c-0305e82c3301\ntype = Model\n");

    ASSERT_TRUE(File::write_all(path, content).has_value());

    auto const read_back = File::read_all(path);
    ASSERT_TRUE(read_back.has_value());
    EXPECT_EQ(read_back.value(), content);
}

TEST_F(FileWrite, TextRoundTripThroughReadLines)
{
    auto const path = path_for("lines.omnia");
    ASSERT_TRUE(File::write_all(path, "[asset]\nuuid = abc\n").has_value());

    auto const lines = File::read_lines(path);
    ASSERT_TRUE(lines.has_value());
    ASSERT_EQ(lines.value().size(), 2U);
    EXPECT_EQ(lines.value()[0], "[asset]");
    EXPECT_EQ(lines.value()[1], "uuid = abc");
}

TEST_F(FileWrite, BinaryRoundTrip)
{
    auto const path = path_for("asset.oasset");

    Binary::ByteWriter writer;
    writer.write<u32>(0xDEADBEEF);
    writer.write_string("payload");

    ASSERT_TRUE(File::write_binary(path, writer.bytes()).has_value());

    auto const read_back = File::read_binary(path);
    ASSERT_TRUE(read_back.has_value());
    EXPECT_EQ(read_back.value().size(), writer.size());

    Binary::ByteReader reader(read_back.value());
    EXPECT_EQ(reader.read<u32>().value(), 0xDEADBEEF);
    EXPECT_EQ(reader.read_string().value(), "payload");
    EXPECT_TRUE(reader.is_exhausted());
}

TEST_F(FileWrite, EmptyBinaryPayload)
{
    auto const path = path_for("empty.oasset");
    ASSERT_TRUE(File::write_binary(path, {}).has_value());

    auto const read_back = File::read_binary(path);
    ASSERT_TRUE(read_back.has_value());
    EXPECT_TRUE(read_back.value().empty());
}

TEST_F(FileWrite, CreatesMissingParentDirectories)
{
    auto const path = path_for("imported") / "nested" / "asset.oasset";
    ASSERT_FALSE(std::filesystem::exists(path.parent_path()));

    ASSERT_TRUE(File::write_all(path, "content").has_value());
    EXPECT_TRUE(std::filesystem::exists(path));
}

TEST_F(FileWrite, OverwriteTruncates)
{
    auto const path = path_for("truncate.txt");
    ASSERT_TRUE(File::write_all(path, "a much longer original content").has_value());
    ASSERT_TRUE(File::write_all(path, "short").has_value());

    auto const read_back = File::read_all(path);
    ASSERT_TRUE(read_back.has_value());
    EXPECT_EQ(read_back.value(), "short");
}

TEST_F(FileWrite, WriteToUnwritablePathFails)
{
    auto const blocker = path_for("blocker");
    ASSERT_TRUE(File::write_all(blocker, "x").has_value());

    EXPECT_FALSE(File::write_all(blocker / "child.txt", "y").has_value());
}
