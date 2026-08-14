/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <format>

#include <Common/Expected.h>
#include <LibAsset/AssetTraits.h>

namespace Asset {

auto AssetTraits<ShaderData>::extensions() -> std::vector<std::string>
{
    return Importer::supported_extensions();
}

auto AssetTraits<ShaderData>::import(ImportContext const& context) -> std::expected<ShaderData, std::string>
{
    return Importer::import(context);
}

auto AssetTraits<ShaderData>::source_hash(ImportContext const& context) -> std::expected<u64, std::string>
{
    return Importer::source_hash(context);
}

auto AssetTraits<TextureData>::extensions() -> std::vector<std::string>
{
    return Importer::supported_extensions();
}

auto AssetTraits<TextureData>::import(ImportContext const& context) -> std::expected<TextureData, std::string>
{
    return Importer::import(context);
}

auto AssetTraits<TextureData>::source_hash(ImportContext const& context) -> std::expected<u64, std::string>
{
    return Importer::source_hash(context);
}

auto AssetTraits<ModelData>::extensions() -> std::vector<std::string>
{
    return Importer::supported_extensions();
}

auto AssetTraits<ModelData>::import(ImportContext const& context) -> std::expected<ModelData, std::string>
{
    return Importer::import(context);
}

auto AssetTraits<ModelData>::source_hash(ImportContext const& context) -> std::expected<u64, std::string>
{
    return Importer::source_hash(context);
}

static_assert(AssetData<ShaderData>);
static_assert(AssetData<TextureData>);
static_assert(AssetData<ModelData>);

// -- Shaders ------------------------------------------------------------------

void AssetTraits<ShaderData>::write(Binary::ByteWriter& writer, ShaderData const& data)
{
    writer.write(static_cast<u8>(data.stage));
    writer.write(static_cast<u64>(data.variants.size()));

    for (auto const& variant : data.variants) {
        writer.write(static_cast<u8>(variant.format));
        writer.write_vector(variant.bytecode);
    }
}

auto AssetTraits<ShaderData>::read(Binary::ByteReader& reader) -> std::expected<ShaderData, std::string>
{
    Graphics::ShaderStage stage {};
    // ShaderStage starts at 1, so 0 is as invalid as anything past Fragment.
    TRY_ASSIGN(stage, reader.read_enum(Graphics::ShaderStage::Vertex, Graphics::ShaderStage::Fragment));

    u64 variant_count {};
    TRY_ASSIGN(variant_count, reader.read<u64>());

    ShaderData data { .stage = stage, .variants = {} };
    for (u64 index = 0; index < variant_count; ++index) {
        Graphics::ShaderFormat format {};
        TRY_ASSIGN(format, reader.read_enum(Graphics::ShaderFormat::SPIRV, Graphics::ShaderFormat::MetalIR));

        std::vector<u8> bytecode {};
        TRY_ASSIGN(bytecode, reader.read_vector<u8>());
        data.variants.push_back({ .format = format, .bytecode = std::move(bytecode) });
    }

    return data;
}

// -- Textures -----------------------------------------------------------------

void AssetTraits<TextureData>::write(Binary::ByteWriter& writer, TextureData const& data)
{
    writer.write(data.width);
    writer.write(data.height);
    writer.write(static_cast<u8>(data.color_space));
    writer.write_vector(data.data);
}

auto AssetTraits<TextureData>::read(Binary::ByteReader& reader) -> std::expected<TextureData, std::string>
{
    u32 width {};
    TRY_ASSIGN(width, reader.read<u32>());

    u32 height {};
    TRY_ASSIGN(height, reader.read<u32>());

    TextureColorSpace color_space {};
    TRY_ASSIGN(color_space, reader.read_enum(TextureColorSpace::Linear, TextureColorSpace::Srgb));

    std::vector<u8> pixels {};
    TRY_ASSIGN(pixels, reader.read_vector<u8>());

    // Always 4 channels as enforced by the importer
    auto const expected_size = static_cast<u64>(width) * height * 4;
    if (pixels.size() != expected_size) {
        return std::unexpected(std::format("Texture is {}x{} so it should hold {} bytes, but the payload has {}.", width, height, expected_size, pixels.size()));
    }

    return TextureData {
        .width = width,
        .height = height,
        .color_space = color_space,
        .data = std::move(pixels)
    };
}

// -- Models -------------------------------------------------------------------

void AssetTraits<ModelData>::write(Binary::ByteWriter& writer, ModelData const& data)
{
    writer.write(static_cast<u64>(data.sub_meshes.size()));
    for (auto const& sub_mesh : data.sub_meshes) {
        writer.write_vector(sub_mesh.vertices);
        writer.write_vector(sub_mesh.indices);
        writer.write(sub_mesh.material_index);
    }

    writer.write(static_cast<u64>(data.materials.size()));
    for (auto const& material : data.materials) {
        writer.write_string(material.name);
        writer.write_optional(material.albedo_texture_id);
        writer.write_optional(material.metallic_roughness_texture_id);
        writer.write_optional(material.normal_texture_id);
        writer.write_optional(material.occlusion_texture_id);
        writer.write_optional(material.emissive_texture_id);
        writer.write(material.parameters);
    }
}

auto AssetTraits<ModelData>::read(Binary::ByteReader& reader) -> std::expected<ModelData, std::string>
{
    u64 sub_mesh_count {};
    TRY_ASSIGN(sub_mesh_count, reader.read<u64>());

    ModelData data;
    for (u64 index = 0; index < sub_mesh_count; ++index) {
        std::vector<Graphics::Vertex> vertices {};
        TRY_ASSIGN(vertices, reader.read_vector<Graphics::Vertex>());

        std::vector<Graphics::Index> indices {};
        TRY_ASSIGN(indices, reader.read_vector<Graphics::Index>());

        u64 material_index {};
        TRY_ASSIGN(material_index, reader.read<u64>());

        data.sub_meshes.push_back({
            .vertices = std::move(vertices),
            .indices = std::move(indices),
            .material_index = material_index,
        });
    }

    u64 material_count {};
    TRY_ASSIGN(material_count, reader.read<u64>());

    for (u64 index = 0; index < material_count; ++index) {
        MaterialData material;

        TRY_ASSIGN(material.name, reader.read_string());

        auto const texture_slots = {
            &MaterialData::albedo_texture_id,
            &MaterialData::metallic_roughness_texture_id,
            &MaterialData::normal_texture_id,
            &MaterialData::occlusion_texture_id,
            &MaterialData::emissive_texture_id
        };
        for (auto const slot : texture_slots) {
            TRY_ASSIGN(material.*slot, reader.read_optional<AssetID>());
        }
        TRY_ASSIGN(material.parameters, reader.read<Graphics::MaterialParameters>());

        data.materials.push_back(std::move(material));
    }

    for (auto const& sub_mesh : data.sub_meshes) {
        if (sub_mesh.material_index >= data.materials.size()) {
            return std::unexpected(std::format("Submesh references material {} but the model only has {}.", sub_mesh.material_index, data.materials.size()));
        }
    }

    return data;
}

}
