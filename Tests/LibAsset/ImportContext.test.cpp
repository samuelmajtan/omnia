/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <gtest/gtest.h>

#include <Common/File.h>
#include <LibAsset/AssetTraits.h>

namespace {

constexpr u8 one_pixel_bmp[] = {
    // File header: "BM", 58 byte file, reserved, pixels start at byte 54.
    0x42, 0x4D, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00,
    // BITMAPINFOHEADER: 40 bytes, 1x1, 1 plane, 24bpp, no compression, 4 byte image.
    0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    // One BGR pixel plus a row-padding byte.
    0x10, 0x20, 0x30, 0x00
};

class Context : public testing::Test {
protected:
    void SetUp() override
    {
        std::error_code error;
        m_directory = std::filesystem::path(testing::TempDir()) / "OmniaImportContextTest";
        std::filesystem::remove_all(m_directory, error);
        std::filesystem::create_directories(m_directory, error);
        ASSERT_FALSE(error) << error.message();

        m_texture = m_directory / "texture.bmp";
        ASSERT_TRUE(File::write_binary(m_texture, std::as_bytes(std::span(one_pixel_bmp))).has_value());
    }

    void TearDown() override
    {
        std::error_code error;
        std::filesystem::remove_all(m_directory, error);
    }

    std::filesystem::path m_directory;
    std::filesystem::path m_texture;
};

}

TEST_F(Context, TextureDecodesWithoutASidecar)
{
    auto const result = Asset::AssetTraits<Asset::TextureData>::import({ .path = m_texture });

    ASSERT_TRUE(result.has_value()) << result.error();
    EXPECT_EQ(result.value().width, 1U);
    EXPECT_EQ(result.value().height, 1U);
    EXPECT_EQ(result.value().color_space, Asset::TextureColorSpace::Linear);
}

TEST_F(Context, TextureDefaultsToLinearWhenTheSettingIsAbsent)
{
    Asset::AssetSidecar const sidecar(Common::UUID::generate(), Asset::AssetType::Texture);

    auto const result = Asset::AssetTraits<Asset::TextureData>::import({ .path = m_texture, .sidecar = &sidecar });

    ASSERT_TRUE(result.has_value()) << result.error();
    EXPECT_EQ(result.value().color_space, Asset::TextureColorSpace::Linear);
}

TEST_F(Context, TextureTakesColorSpaceFromTheSidecar)
{
    Asset::AssetSidecar sidecar(Common::UUID::generate(), Asset::AssetType::Texture);
    sidecar.set_setting("srgb", "true");

    auto const result = Asset::AssetTraits<Asset::TextureData>::import({ .path = m_texture, .sidecar = &sidecar });

    ASSERT_TRUE(result.has_value()) << result.error();
    EXPECT_EQ(result.value().color_space, Asset::TextureColorSpace::Srgb);
}

TEST_F(Context, TextureSettingSurvivesARoundTripThroughTheCookedBlob)
{
    Asset::AssetSidecar sidecar(Common::UUID::generate(), Asset::AssetType::Texture);
    sidecar.set_setting("srgb", "true");

    auto const imported = Asset::AssetTraits<Asset::TextureData>::import({ .path = m_texture, .sidecar = &sidecar });
    ASSERT_TRUE(imported.has_value()) << imported.error();

    Binary::ByteWriter writer;
    Asset::AssetTraits<Asset::TextureData>::write(writer, imported.value());

    Binary::ByteReader reader(writer.bytes());
    auto const read_back = Asset::AssetTraits<Asset::TextureData>::read(reader);
    ASSERT_TRUE(read_back.has_value()) << read_back.error();
    EXPECT_EQ(read_back.value().color_space, Asset::TextureColorSpace::Srgb);
}

TEST_F(Context, ModelRefusesToImportWithoutARegistry)
{
    auto const result = Asset::AssetTraits<Asset::ModelData>::import({ .path = m_directory / "model.gltf" });
    EXPECT_FALSE(result.has_value());
}

TEST_F(Context, ShaderIgnoresTheRestOfTheContext)
{
    auto const shader = m_directory / "Passthrough.vs.glsl";
    ASSERT_TRUE(File::write_all(shader, "#version 450 core\nvoid main() { gl_Position = vec4(0.0); }\n").has_value());

    auto const result = Asset::AssetTraits<Asset::ShaderData>::import({ .path = shader });
    ASSERT_TRUE(result.has_value()) << result.error();
    EXPECT_EQ(result.value().stage, Graphics::ShaderStage::Vertex);
    ASSERT_EQ(result.value().variants.size(), 1U);
    EXPECT_FALSE(result.value().variants[0].bytecode.empty());
}

TEST_F(Context, ShaderSourceHashCoversIncludes)
{
    auto const shader = m_directory / "Uses.fs.glsl";
    auto const include = m_directory / "Shared.glsl";

    ASSERT_TRUE(File::write_all(include, "#define TINT 1.0\n").has_value());
    ASSERT_TRUE(File::write_all(shader, "#include <Shared.glsl>\nvoid main() {}\n").has_value());

    auto const before = Asset::AssetTraits<Asset::ShaderData>::source_hash({ .path = shader });
    ASSERT_TRUE(before.has_value()) << before.error();

    ASSERT_TRUE(File::write_all(include, "#define TINT 2.0\n").has_value());

    auto const after = Asset::AssetTraits<Asset::ShaderData>::source_hash({ .path = shader });
    ASSERT_TRUE(after.has_value()) << after.error();

    EXPECT_NE(before.value(), after.value()) << "editing an include must invalidate the shader that includes it";
}

TEST_F(Context, ShaderSourceHashIsStableWhenNothingChanges)
{
    auto const shader = m_directory / "Stable.vs.glsl";
    ASSERT_TRUE(File::write_all(shader, "void main() {}\n").has_value());

    auto const first = Asset::AssetTraits<Asset::ShaderData>::source_hash({ .path = shader });
    auto const second = Asset::AssetTraits<Asset::ShaderData>::source_hash({ .path = shader });

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first.value(), second.value());
}
