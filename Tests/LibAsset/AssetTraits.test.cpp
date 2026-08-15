/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <cstring>

#include <gtest/gtest.h>

#include <Common/Expected.h>
#include <LibAsset/AssetFile.h>
#include <LibAsset/AssetTraits.h>

namespace {

template<typename T>
auto same_bytes(std::vector<T> const& lhs, std::vector<T> const& rhs) -> bool
{
    return lhs.size() == rhs.size() && (lhs.empty() || std::memcmp(lhs.data(), rhs.data(), lhs.size() * sizeof(T)) == 0);
}

auto make_vertex(f32 seed) -> Graphics::Vertex
{
    return Graphics::Vertex {
        .position = { seed, seed + 1.0F, seed + 2.0F },
        .tex_coord = { seed + 3.0F, seed + 4.0F },
        .normal = { 0.0F, 1.0F, 0.0F },
        .tangent = { 1.0F, 0.0F, 0.0F, -1.0F }
    };
}

auto make_model() -> Asset::ModelData
{
    Asset::MaterialData material;
    material.name = "Fabric";
    material.albedo_texture_id = Common::UUID::generate();
    material.normal_texture_id = Common::UUID::generate();
    material.parameters.base_color = { 0.5F, 0.25F, 0.125F, 1.0F };
    material.parameters.metallic_factor = 0.75F;
    material.parameters.roughness_factor = 0.375F;

    Asset::MaterialData untextured;
    untextured.name = "Plain";

    return Asset::ModelData {
        .sub_meshes = {
            { .vertices = { make_vertex(0.0F), make_vertex(10.0F), make_vertex(20.0F) }, .indices = { 0, 1, 2 }, .material_index = 0 },
            { .vertices = { make_vertex(30.0F) }, .indices = { 0, 0, 0 }, .material_index = 1 },
        },
        .materials = { material, untextured }
    };
}

template<typename T>
auto round_trip(T const& value) -> Common::Expected<T>
{
    Binary::ByteWriter writer;
    Asset::AssetTraits<T>::write(writer, value);

    Binary::ByteReader reader(writer.bytes());
    return Asset::AssetTraits<T>::read(reader);
}

}

TEST(SerializeShader, RoundTrip)
{
    Asset::ShaderData const original {
        .stage = Graphics::ShaderStage::Fragment,
        .variants = {
            { .format = Graphics::ShaderFormat::SPIRV, .bytecode = { 0x03, 0x02, 0x23, 0x07 } },
            { .format = Graphics::ShaderFormat::DXIL, .bytecode = { 0xFF, 0x00 } },
        }
    };

    auto const result = round_trip(original);
    ASSERT_TRUE(result.has_value()) << result.error();
    EXPECT_EQ(result.value().stage, original.stage);
    ASSERT_EQ(result.value().variants.size(), 2U);
    EXPECT_EQ(result.value().variants[0].format, Graphics::ShaderFormat::SPIRV);
    EXPECT_TRUE(same_bytes(result.value().variants[0].bytecode, original.variants[0].bytecode));
    EXPECT_EQ(result.value().variants[1].format, Graphics::ShaderFormat::DXIL);
    EXPECT_TRUE(same_bytes(result.value().variants[1].bytecode, original.variants[1].bytecode));
}

TEST(SerializeShader, NoVariantsRoundTrips)
{
    auto const result = round_trip(Asset::ShaderData { .stage = Graphics::ShaderStage::Vertex, .variants = {} });
    ASSERT_TRUE(result.has_value()) << result.error();
    EXPECT_TRUE(result.value().variants.empty());
}

TEST(SerializeShader, InvalidStageIsRejected)
{
    for (u8 const stage : { 0, 3, 200 }) {
        Binary::ByteWriter writer;
        writer.write<u8>(stage);
        writer.write<u64>(0);

        Binary::ByteReader reader(writer.bytes());
        EXPECT_FALSE(Asset::AssetTraits<Asset::ShaderData>::read(reader).has_value()) << "stage " << int(stage);
    }
}

TEST(SerializeShader, InvalidFormatIsRejected)
{
    Binary::ByteWriter writer;
    writer.write<u8>(static_cast<u8>(Graphics::ShaderStage::Vertex));
    writer.write<u64>(1);
    writer.write<u8>(77);
    writer.write_vector(std::vector<u8> { 1, 2 });

    Binary::ByteReader reader(writer.bytes());
    EXPECT_FALSE(Asset::AssetTraits<Asset::ShaderData>::read(reader).has_value());
}

TEST(SerializeShader, TruncatedPayloadIsRejected)
{
    Binary::ByteWriter writer;
    Asset::AssetTraits<Asset::ShaderData>::write(writer, { .stage = Graphics::ShaderStage::Vertex, .variants = { { Graphics::ShaderFormat::SPIRV, { 1, 2, 3, 4 } } } });

    Binary::ByteReader reader(writer.bytes().first(writer.size() - 2));
    EXPECT_FALSE(Asset::AssetTraits<Asset::ShaderData>::read(reader).has_value());
}

TEST(SerializeTexture, RoundTrip)
{
    Asset::TextureData const original {
        .width = 2,
        .height = 2,
        .color_space = Asset::TextureColorSpace::Srgb,
        .data = std::vector<u8>(2 * 2 * 4, 0xAB)
    };

    auto const result = round_trip(original);
    ASSERT_TRUE(result.has_value()) << result.error();
    EXPECT_EQ(result.value().width, 2U);
    EXPECT_EQ(result.value().height, 2U);
    EXPECT_EQ(result.value().color_space, Asset::TextureColorSpace::Srgb);
    EXPECT_TRUE(same_bytes(result.value().data, original.data));
}

TEST(SerializeTexture, LinearColorSpaceRoundTrips)
{
    auto const result = round_trip(Asset::TextureData { .width = 1, .height = 1, .color_space = Asset::TextureColorSpace::Linear, .data = { 1, 2, 3, 4 } });
    ASSERT_TRUE(result.has_value()) << result.error();
    EXPECT_EQ(result.value().color_space, Asset::TextureColorSpace::Linear);
}

TEST(SerializeTexture, DimensionsMustMatchPayload)
{
    Binary::ByteWriter writer;
    writer.write<u32>(64);
    writer.write<u32>(64);
    writer.write<u8>(static_cast<u8>(Asset::TextureColorSpace::Linear));
    writer.write_vector(std::vector<u8> { 1, 2, 3, 4 });

    Binary::ByteReader reader(writer.bytes());
    EXPECT_FALSE(Asset::AssetTraits<Asset::TextureData>::read(reader).has_value());
}

TEST(SerializeTexture, InvalidColorSpaceIsRejected)
{
    Binary::ByteWriter writer;
    writer.write<u32>(1);
    writer.write<u32>(1);
    writer.write<u8>(42);
    writer.write_vector(std::vector<u8> { 1, 2, 3, 4 });

    Binary::ByteReader reader(writer.bytes());
    EXPECT_FALSE(Asset::AssetTraits<Asset::TextureData>::read(reader).has_value());
}

TEST(SerializeModel, RoundTrip)
{
    auto const original = make_model();

    auto const result = round_trip(original);
    ASSERT_TRUE(result.has_value()) << result.error();
    auto const& model = result.value();

    ASSERT_EQ(model.sub_meshes.size(), original.sub_meshes.size());
    for (std::size_t index = 0; index < model.sub_meshes.size(); ++index) {
        EXPECT_TRUE(same_bytes(model.sub_meshes[index].vertices, original.sub_meshes[index].vertices)) << "submesh " << index;
        EXPECT_TRUE(same_bytes(model.sub_meshes[index].indices, original.sub_meshes[index].indices)) << "submesh " << index;
        EXPECT_EQ(model.sub_meshes[index].material_index, original.sub_meshes[index].material_index);
    }

    ASSERT_EQ(model.materials.size(), 2U);
    EXPECT_EQ(model.materials[0].name, "Fabric");
    EXPECT_EQ(model.materials[0].albedo_texture_id, original.materials[0].albedo_texture_id);
    EXPECT_EQ(model.materials[0].normal_texture_id, original.materials[0].normal_texture_id);
    EXPECT_EQ(model.materials[0].metallic_roughness_texture_id, std::nullopt);
    EXPECT_EQ(model.materials[0].parameters.metallic_factor, 0.75F);
    EXPECT_EQ(model.materials[0].parameters.roughness_factor, 0.375F);
    EXPECT_EQ(model.materials[0].parameters.base_color.x, 0.5F);

    EXPECT_EQ(model.materials[1].name, "Plain");
    EXPECT_EQ(model.materials[1].albedo_texture_id, std::nullopt);
}

TEST(SerializeModel, TextureSlotsDoNotGetSwapped)
{
    Asset::MaterialData material;
    material.name = "Slots";
    material.metallic_roughness_texture_id = Common::UUID::generate();
    material.occlusion_texture_id = Common::UUID::generate();

    auto const result = round_trip(Asset::ModelData {
        .sub_meshes = { { .vertices = { make_vertex(0.0F) }, .indices = { 0 }, .material_index = 0 } },
        .materials = { material } });

    ASSERT_TRUE(result.has_value()) << result.error();
    auto const& read_back = result.value().materials[0];
    EXPECT_EQ(read_back.albedo_texture_id, std::nullopt);
    EXPECT_EQ(read_back.metallic_roughness_texture_id, material.metallic_roughness_texture_id);
    EXPECT_EQ(read_back.normal_texture_id, std::nullopt);
    EXPECT_EQ(read_back.occlusion_texture_id, material.occlusion_texture_id);
    EXPECT_EQ(read_back.emissive_texture_id, std::nullopt);
}

TEST(SerializeModel, EmptyModelRoundTrips)
{
    auto const result = round_trip(Asset::ModelData {});
    ASSERT_TRUE(result.has_value()) << result.error();
    EXPECT_TRUE(result.value().sub_meshes.empty());
    EXPECT_TRUE(result.value().materials.empty());
}

TEST(SerializeModel, OutOfRangeMaterialIndexIsRejected)
{
    Binary::ByteWriter writer;
    writer.write<u64>(1);
    writer.write_vector(std::vector<Graphics::Vertex> { make_vertex(0.0F) });
    writer.write_vector(std::vector<Graphics::Index> { 0 });
    writer.write<u64>(9);
    writer.write<u64>(1);
    writer.write_string("OnlyMaterial");
    for (int slot = 0; slot < 5; ++slot) {
        writer.write_optional(std::optional<Asset::AssetID> {});
    }
    writer.write(Graphics::MaterialParameters {});

    Binary::ByteReader reader(writer.bytes());
    EXPECT_FALSE(Asset::AssetTraits<Asset::ModelData>::read(reader).has_value());
}

TEST(SerializeModel, TruncatedPayloadIsRejected)
{
    Binary::ByteWriter writer;
    Asset::AssetTraits<Asset::ModelData>::write(writer, make_model());

    for (auto const fraction : { 0.25, 0.5, 0.75, 0.95 }) {
        auto const length = static_cast<std::size_t>(static_cast<double>(writer.size()) * fraction);
        Binary::ByteReader reader(writer.bytes().first(length));
        EXPECT_FALSE(Asset::AssetTraits<Asset::ModelData>::read(reader).has_value()) << "truncated to " << fraction;
    }
}

TEST(AssetFileHeader, RoundTrip)
{
    Asset::AssetFileHeader const original {
        .format_version = Asset::AssetFileHeader::VERSION,
        .asset_type = Asset::AssetType::Model,
        .importer_version = 3,
        .source_hash = 0x0123456789ABCDEFULL,
        .compression = Asset::CompressionMode::None,
        .payload_size = 0
    };

    Binary::ByteWriter writer;
    original.write(writer);

    Binary::ByteReader reader(writer.bytes());
    auto const result = Asset::AssetFileHeader::read(reader);
    ASSERT_TRUE(result.has_value()) << result.error();
    EXPECT_EQ(result.value().asset_type, Asset::AssetType::Model);
    EXPECT_EQ(result.value().importer_version, 3U);
    EXPECT_EQ(result.value().source_hash, 0x0123456789ABCDEFULL);
}

TEST(AssetFileHeader, MatchesOnlyWhenEverythingAgrees)
{
    Asset::AssetFileHeader const header {
        .asset_type = Asset::AssetType::Texture,
        .importer_version = 2,
        .source_hash = 1234
    };

    EXPECT_TRUE(header.matches(Asset::AssetType::Texture, 2, 1234));
    EXPECT_FALSE(header.matches(Asset::AssetType::Model, 2, 1234));
    EXPECT_FALSE(header.matches(Asset::AssetType::Texture, 3, 1234));
    EXPECT_FALSE(header.matches(Asset::AssetType::Texture, 2, 5678));
}

TEST(AssetFileHeader, RejectsForeignFile)
{
    Binary::ByteWriter writer;
    writer.write<u64>(0xDEADBEEFDEADBEEFULL);

    Binary::ByteReader reader(writer.bytes());
    EXPECT_FALSE(Asset::AssetFileHeader::read(reader).has_value());
}

TEST(AssetFileHeader, RejectsFutureFormatVersion)
{
    Binary::ByteWriter writer;
    writer.write<u64>(Asset::AssetFileHeader::MAGIC);
    writer.write<u32>(Asset::AssetFileHeader::VERSION + 1);
    writer.write<u8>(0);
    writer.write<u32>(1);
    writer.write<u8>(0);
    writer.write<u64>(0);
    writer.write<u64>(0);

    Binary::ByteReader reader(writer.bytes());
    EXPECT_FALSE(Asset::AssetFileHeader::read(reader).has_value());
}

TEST(AssetFileHeader, RejectsUnknownAssetTypeAndCompression)
{
    auto const build = [](u32 asset_type, u32 compression) {
        Binary::ByteWriter writer;
        writer.write<u64>(Asset::AssetFileHeader::MAGIC);
        writer.write<u32>(Asset::AssetFileHeader::VERSION);
        writer.write<u8>(asset_type);
        writer.write<u32>(1);
        writer.write<u8>(compression);
        writer.write<u64>(0);
        writer.write<u64>(0);
        return writer;
    };

    auto const bad_type = build(99, 0);
    Binary::ByteReader type_reader(bad_type.bytes());
    EXPECT_FALSE(Asset::AssetFileHeader::read(type_reader).has_value());

    auto const bad_compression = build(0, 7);
    Binary::ByteReader compression_reader(bad_compression.bytes());
    EXPECT_FALSE(Asset::AssetFileHeader::read(compression_reader).has_value());
}

TEST(AssetFileHeader, RejectsPayloadSizeLongerThanTheFile)
{
    Binary::ByteWriter writer;
    writer.write<u64>(Asset::AssetFileHeader::MAGIC);
    writer.write<u32>(Asset::AssetFileHeader::VERSION);
    writer.write<u8>(static_cast<u8>(Asset::AssetType::Shader));
    writer.write<u32>(1);
    writer.write<u8>(0);
    writer.write<u64>(0);
    writer.write<u64>(4096);

    Binary::ByteReader reader(writer.bytes());
    EXPECT_FALSE(Asset::AssetFileHeader::read(reader).has_value());
}
