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

auto AssetTraits<ShaderData>::import(ImportContext const& context) -> Common::Expected<ShaderData>
{
    return Importer::import(context);
}

auto AssetTraits<ShaderData>::source_hash(ImportContext const& context) -> Common::Expected<u64>
{
    return Importer::source_hash(context);
}

auto AssetTraits<TextureData>::extensions() -> std::vector<std::string>
{
    return Importer::supported_extensions();
}

auto AssetTraits<TextureData>::import(ImportContext const& context) -> Common::Expected<TextureData>
{
    return Importer::import(context);
}

auto AssetTraits<TextureData>::source_hash(ImportContext const& context) -> Common::Expected<u64>
{
    return Importer::source_hash(context);
}

auto AssetTraits<ModelData>::extensions() -> std::vector<std::string>
{
    return Importer::supported_extensions();
}

auto AssetTraits<ModelData>::import(ImportContext const& context) -> Common::Expected<ModelData>
{
    return Importer::import(context);
}

auto AssetTraits<ModelData>::source_hash(ImportContext const& context) -> Common::Expected<u64>
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

auto AssetTraits<ShaderData>::read(Binary::ByteReader& reader) -> Common::Expected<ShaderData>
{
    auto const stage = TRY(reader.read_enum(Graphics::ShaderStage::Vertex, Graphics::ShaderStage::Fragment));
    auto const variant_count = TRY(reader.read<u64>());

    ShaderData data { .stage = stage, .variants = {} };
    for (u64 index = 0; index < variant_count; ++index) {
        auto const format = TRY(reader.read_enum(Graphics::ShaderFormat::SPIRV, Graphics::ShaderFormat::MetalIR));
        auto const bytecode = TRY(reader.read_vector<u8>());
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

auto AssetTraits<TextureData>::read(Binary::ByteReader& reader) -> Common::Expected<TextureData>
{
    auto const width = TRY(reader.read<u32>());
    auto const height = TRY(reader.read<u32>());
    auto const color_space = TRY(reader.read_enum(TextureColorSpace::Linear, TextureColorSpace::Srgb));
    auto const pixels = TRY(reader.read_vector<u8>());

    // Always 4 channels as enforced by the importer
    auto const expected_size = static_cast<u64>(width) * height * 4;
    if (pixels.size() != expected_size) {
        return OA_ERROR("Texture is {}x{} so it should hold {} bytes, but the payload has {}", width, height, expected_size, pixels.size());
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

auto AssetTraits<ModelData>::read(Binary::ByteReader& reader) -> Common::Expected<ModelData>
{
    auto const sub_mesh_count = TRY(reader.read<u64>());

    ModelData data;
    for (u64 index = 0; index < sub_mesh_count; ++index) {
        data.sub_meshes.push_back({
            .vertices = TRY(reader.read_vector<Graphics::Vertex>()),
            .indices = TRY(reader.read_vector<Graphics::Index>()),
            .material_index = TRY(reader.read<u64>()),
        });
    }

    auto const material_count = TRY(reader.read<u64>());
    data.materials.reserve(material_count);
    for (u64 index = 0; index < material_count; ++index) {
        MaterialData material;
        material.name = TRY(reader.read_string());

        auto const texture_slots = {
            &MaterialData::albedo_texture_id,
            &MaterialData::metallic_roughness_texture_id,
            &MaterialData::normal_texture_id,
            &MaterialData::occlusion_texture_id,
            &MaterialData::emissive_texture_id
        };
        for (auto const slot : texture_slots) {
            material.*slot = TRY(reader.read_optional<AssetID>());
        }
        material.parameters = TRY(reader.read<Graphics::MaterialParameters>());
        data.materials.push_back(std::move(material));
    }

    for (auto const& sub_mesh : data.sub_meshes) {
        if (sub_mesh.material_index >= data.materials.size()) {
            return OA_ERROR("Submesh references material {} but the model only has {}", sub_mesh.material_index, data.materials.size());
        }
    }

    return data;
}

}
