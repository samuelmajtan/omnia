/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <gtest/gtest.h>

#include <Common/File.h>
#include <LibAsset/AssetSidecar.h>

namespace {

class Sidecar : public testing::Test {
protected:
    void SetUp() override
    {
        std::error_code error;
        m_directory = std::filesystem::path(testing::TempDir()) / "OmniaSidecarTest";
        std::filesystem::remove_all(m_directory, error);
        std::filesystem::create_directories(m_directory, error);
        ASSERT_FALSE(error) << error.message();
    }

    void TearDown() override
    {
        std::error_code error;
        std::filesystem::remove_all(m_directory, error);
    }

    auto write_sidecar(std::string_view content) const -> std::filesystem::path
    {
        auto const path = m_directory / "asset.png.omnia";
        auto const result = File::write_all(path, content);
        EXPECT_TRUE(result.has_value());
        return path;
    }

    std::filesystem::path m_directory;
};

}

TEST_F(Sidecar, PathIsAppendedToTheFullFilename)
{
    EXPECT_EQ(Asset::AssetSidecar::path_for("Models/Sponza.gltf").generic_string(), "Models/Sponza.gltf.omnia");
    EXPECT_EQ(Asset::AssetSidecar::path_for("Shaders/GeometryPass.vs.glsl").generic_string(), "Shaders/GeometryPass.vs.glsl.omnia");
}

TEST_F(Sidecar, RoundTrip)
{
    auto const path = m_directory / "asset.png.omnia";
    auto const id = Common::UUID::generate();

    Asset::AssetSidecar written(id, Asset::AssetType::Texture);
    written.set_setting("srgb", "true");
    ASSERT_TRUE(written.save(path).has_value());

    auto const read_back = Asset::AssetSidecar::load(path);
    ASSERT_TRUE(read_back.has_value()) << read_back.error();
    EXPECT_EQ(read_back.value().id(), id);
    EXPECT_EQ(read_back.value().type(), Asset::AssetType::Texture);
    EXPECT_TRUE(read_back.value().bool_setting("srgb", false));
}

TEST_F(Sidecar, AllAssetTypesRoundTrip)
{
    for (auto const type : { Asset::AssetType::Model, Asset::AssetType::Texture, Asset::AssetType::Shader }) {
        auto const path = m_directory / "asset.omnia";
        ASSERT_TRUE(Asset::AssetSidecar(Common::UUID::generate(), type).save(path).has_value());

        auto const read_back = Asset::AssetSidecar::load(path);
        ASSERT_TRUE(read_back.has_value()) << read_back.error();
        EXPECT_EQ(read_back.value().type(), type);
    }
}

TEST_F(Sidecar, IgnoresCommentsAndBlankLines)
{
    auto const path = write_sidecar("# a comment\n\n  \nuuid = 3f2504e0-4f89-11d3-9a0c-0305e82c3301\ntype = Texture\n");

    auto const sidecar = Asset::AssetSidecar::load(path);
    ASSERT_TRUE(sidecar.has_value()) << sidecar.error();
    EXPECT_EQ(sidecar.value().type(), Asset::AssetType::Texture);
}

TEST_F(Sidecar, ToleratesCarriageReturns)
{
    auto const path = write_sidecar("uuid = 3f2504e0-4f89-11d3-9a0c-0305e82c3301\r\ntype = Shader\r\n");

    auto const sidecar = Asset::AssetSidecar::load(path);
    ASSERT_TRUE(sidecar.has_value()) << sidecar.error();
    EXPECT_EQ(sidecar.value().type(), Asset::AssetType::Shader);
}

TEST_F(Sidecar, ToleratesSurroundingWhitespace)
{
    auto const path = write_sidecar("   uuid   =   3f2504e0-4f89-11d3-9a0c-0305e82c3301   \n\ttype\t=\tModel\n");

    auto const sidecar = Asset::AssetSidecar::load(path);
    ASSERT_TRUE(sidecar.has_value()) << sidecar.error();
    EXPECT_EQ(sidecar.value().type(), Asset::AssetType::Model);
}

TEST_F(Sidecar, UnknownKeysBecomeSettings)
{
    auto const path = write_sidecar("uuid = 3f2504e0-4f89-11d3-9a0c-0305e82c3301\ntype = Texture\nsrgb = false\ngenerate_mips = true\n");

    auto const sidecar = Asset::AssetSidecar::load(path);
    ASSERT_TRUE(sidecar.has_value()) << sidecar.error();
    EXPECT_FALSE(sidecar.value().bool_setting("srgb", true));
    EXPECT_TRUE(sidecar.value().bool_setting("generate_mips", false));
    EXPECT_EQ(sidecar.value().setting("nothing_here"), std::nullopt);
    EXPECT_TRUE(sidecar.value().bool_setting("nothing_here", true));
}

TEST_F(Sidecar, MissingUuidIsRejected)
{
    auto const path = write_sidecar("type = Texture\n");
    EXPECT_FALSE(Asset::AssetSidecar::load(path).has_value());
}

TEST_F(Sidecar, MissingTypeIsRejected)
{
    auto const path = write_sidecar("uuid = 3f2504e0-4f89-11d3-9a0c-0305e82c3301\n");
    EXPECT_FALSE(Asset::AssetSidecar::load(path).has_value());
}

TEST_F(Sidecar, MalformedUuidIsRejected)
{
    auto const path = write_sidecar("uuid = not-a-uuid\ntype = Texture\n");
    EXPECT_FALSE(Asset::AssetSidecar::load(path).has_value());
}

TEST_F(Sidecar, UnknownTypeIsRejected)
{
    auto const path = write_sidecar("uuid = 3f2504e0-4f89-11d3-9a0c-0305e82c3301\ntype = Hologram\n");
    EXPECT_FALSE(Asset::AssetSidecar::load(path).has_value());
}

TEST_F(Sidecar, LineWithoutSeparatorIsRejected)
{
    auto const path = write_sidecar("uuid = 3f2504e0-4f89-11d3-9a0c-0305e82c3301\ntype = Texture\njust some noise\n");
    EXPECT_FALSE(Asset::AssetSidecar::load(path).has_value());
}

TEST_F(Sidecar, MissingFileIsRejected)
{
    EXPECT_FALSE(Asset::AssetSidecar::load(m_directory / "absent.omnia").has_value());
}

TEST_F(Sidecar, SaveIsDeterministic)
{
    auto const path = m_directory / "asset.omnia";

    Asset::AssetSidecar sidecar(Common::UUID::generate(), Asset::AssetType::Texture);
    sidecar.set_setting("zebra", "1");
    sidecar.set_setting("alpha", "2");
    sidecar.set_setting("middle", "3");

    ASSERT_TRUE(sidecar.save(path).has_value());
    auto const first = File::read_all(path);

    ASSERT_TRUE(sidecar.save(path).has_value());
    auto const second = File::read_all(path);

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first.value(), second.value());
    EXPECT_LT(first.value().find("alpha"), first.value().find("middle"));
    EXPECT_LT(first.value().find("middle"), first.value().find("zebra"));
}
