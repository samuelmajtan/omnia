/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <algorithm>
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

auto AssetTraits<ShaderData>::enumerate_sub_assets(std::filesystem::path const&) -> Common::Expected<std::vector<SubAssetDescriptor>>
{
    return std::vector<SubAssetDescriptor> {};
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

auto AssetTraits<TextureData>::enumerate_sub_assets(std::filesystem::path const&) -> Common::Expected<std::vector<SubAssetDescriptor>>
{
    return std::vector<SubAssetDescriptor> {};
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

auto AssetTraits<ModelData>::enumerate_sub_assets(std::filesystem::path const& path) -> Common::Expected<std::vector<SubAssetDescriptor>>
{
    return Importer::enumerate_sub_assets(path);
}

auto AssetTraits<AnimationData>::extensions() -> std::vector<std::string>
{
    return Importer::supported_extensions();
}

auto AssetTraits<AnimationData>::import(ImportContext const& context) -> Common::Expected<AnimationData>
{
    return Importer::import(context);
}

auto AssetTraits<AnimationData>::source_hash(ImportContext const& context) -> Common::Expected<u64>
{
    return Importer::source_hash(context);
}

auto AssetTraits<AnimationData>::enumerate_sub_assets(std::filesystem::path const&) -> Common::Expected<std::vector<SubAssetDescriptor>>
{
    return std::vector<SubAssetDescriptor> {};
}

static_assert(AssetData<ShaderData>);
static_assert(AssetData<TextureData>);
static_assert(AssetData<ModelData>);
static_assert(AssetData<AnimationData>);

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
    auto const stage = TRY(reader.read_enum<Graphics::ShaderStage>());
    auto const variant_count = TRY(reader.read<u64>());

    ShaderData data { .stage = stage, .variants = {} };
    for (u64 index = 0; index < variant_count; ++index) {
        auto const format = TRY(reader.read_enum<Graphics::ShaderFormat>());
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
    auto const color_space = TRY(reader.read_enum<TextureColorSpace>());
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
        writer.write_vector(sub_mesh.skinned_vertices);
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

    writer.write(static_cast<u8>(data.skeleton.has_value() ? 1 : 0));
    if (data.skeleton.has_value()) {
        auto const& skeleton = data.skeleton.value();

        writer.write(static_cast<u64>(skeleton.nodes.size()));
        for (auto const& node : skeleton.nodes) {
            writer.write_string(node.name);
            writer.write(node.parent_index);
            writer.write(node.translation);
            writer.write(node.rotation);
            writer.write(node.scale);
        }

        writer.write_vector(skeleton.bone_nodes);
        writer.write_vector(skeleton.inverse_bind_matrices);
    }
}

auto AssetTraits<ModelData>::read(Binary::ByteReader& reader) -> Common::Expected<ModelData>
{
    auto const sub_mesh_count = TRY(reader.read<u64>());

    ModelData data;
    for (u64 index = 0; index < sub_mesh_count; ++index) {
        data.sub_meshes.push_back({
            .vertices = TRY(reader.read_vector<Graphics::Vertex>()),
            .skinned_vertices = TRY(reader.read_vector<Graphics::SkinnedVertex>()),
            .indices = TRY(reader.read_vector<Graphics::Index>()),
            .material_index = TRY(reader.read<u64>()),
        });

        auto const& sub_mesh = data.sub_meshes.back();
        if (!sub_mesh.vertices.empty() && !sub_mesh.skinned_vertices.empty()) {
            return OA_ERROR("Submesh {} holds both {} plain and {} skinned vertices, it must hold one or the other", index, sub_mesh.vertices.size(), sub_mesh.skinned_vertices.size());
        }
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

    if (TRY(reader.read<u8>()) != 0) {
        Graphics::SkeletonData skeleton;

        auto const node_count = TRY(reader.read<u64>());
        skeleton.nodes.reserve(node_count);
        for (u64 index = 0; index < node_count; ++index) {
            Graphics::SkeletonNode node;
            node.name = TRY(reader.read_string());
            node.parent_index = TRY(reader.read<i32>());
            node.translation = TRY(reader.read<Math::Vec3f>());
            node.rotation = TRY(reader.read<Math::Quatf>());
            node.scale = TRY(reader.read<Math::Vec3f>());

            if (node.parent_index < -1 || node.parent_index >= static_cast<i32>(index)) {
                return OA_ERROR("Skeleton node {} has parent {}, which breaks the parent-before-child ordering", index, node.parent_index);
            }
            skeleton.nodes.push_back(std::move(node));
        }

        skeleton.bone_nodes = TRY(reader.read_vector<u32>());
        skeleton.inverse_bind_matrices = TRY(reader.read_vector<Math::Mat4f>());

        if (skeleton.bone_nodes.size() != skeleton.inverse_bind_matrices.size()) {
            return OA_ERROR("Skeleton has {} bones but {} inverse bind matrices", skeleton.bone_nodes.size(), skeleton.inverse_bind_matrices.size());
        }
        for (auto const bone_node : skeleton.bone_nodes) {
            if (bone_node >= skeleton.nodes.size()) {
                return OA_ERROR("Skeleton bone points at node {} but the hierarchy only has {}", bone_node, skeleton.nodes.size());
            }
        }
        data.skeleton = std::move(skeleton);
    }

    for (auto const& sub_mesh : data.sub_meshes) {
        if (sub_mesh.material_index >= data.materials.size()) {
            return OA_ERROR("Submesh references material {} but the model only has {}", sub_mesh.material_index, data.materials.size());
        }
        if (!sub_mesh.is_skinned()) {
            continue;
        }

        auto const bone_count = data.skeleton.has_value() ? data.skeleton->bone_nodes.size() : 0;
        for (auto const& vertex : sub_mesh.skinned_vertices) {
            auto const& bones = vertex.bone_indices;
            if (bones.x >= bone_count || bones.y >= bone_count || bones.z >= bone_count || bones.w >= bone_count) {
                return OA_ERROR("A skinned vertex references a bone outside the skin's {} bones", bone_count);
            }
        }
    }

    return data;
}

// -- Animations ---------------------------------------------------------------

void AssetTraits<AnimationData>::write(Binary::ByteWriter& writer, AnimationData const& data)
{
    writer.write_string(data.name);
    writer.write(data.duration);

    writer.write(static_cast<u64>(data.channels.size()));
    for (auto const& channel : data.channels) {
        writer.write(channel.node_index);
        writer.write(static_cast<u8>(channel.path));
        writer.write(static_cast<u8>(channel.interpolation));
        writer.write_vector(channel.times);
        writer.write_vector(channel.values);
    }
}

auto AssetTraits<AnimationData>::read(Binary::ByteReader& reader) -> Common::Expected<AnimationData>
{
    AnimationData data;
    data.name = TRY(reader.read_string());
    data.duration = TRY(reader.read<f32>());

    if (data.duration < 0.0F) {
        return OA_ERROR("Animation '{}' has a duration of {}", data.name, data.duration);
    }

    auto const channel_count = TRY(reader.read<u64>());
    data.channels.reserve(channel_count);
    for (u64 index = 0; index < channel_count; ++index) {
        AnimationChannel channel;
        channel.node_index = TRY(reader.read<u32>());
        channel.path = TRY(reader.read_enum<AnimationPath>());
        channel.interpolation = TRY(reader.read_enum<AnimationInterpolation>());
        channel.times = TRY(reader.read_vector<f32>());
        channel.values = TRY(reader.read_vector<f32>());

        auto const components = components_for(channel.path);
        if (channel.values.size() != channel.times.size() * components) {
            return OA_ERROR("Animation '{}' has a channel with {} keyframe times but {} values, expected {} per key",
                data.name, channel.times.size(), channel.values.size(), components);
        }
        if (!std::ranges::is_sorted(channel.times)) {
            return OA_ERROR("Animation '{}' has a channel whose keyframe times are not ascending", data.name);
        }
        data.channels.push_back(std::move(channel));
    }

    return data;
}

auto asset_type_for(std::filesystem::path const& path) -> std::optional<AssetType>
{
    if (claims_extension(path, AssetTraits<ModelData>::extensions())) {
        return AssetTraits<ModelData>::TYPE;
    }
    if (claims_extension(path, AssetTraits<TextureData>::extensions())) {
        return AssetTraits<TextureData>::TYPE;
    }
    if (claims_extension(path, AssetTraits<ShaderData>::extensions())) {
        return AssetTraits<ShaderData>::TYPE;
    }
    return std::nullopt;
}

auto sub_assets_for(AssetType type, std::filesystem::path const& path) -> Common::Expected<std::vector<SubAssetDescriptor>>
{
    switch (type) {
    case AssetType::Model:
        return AssetTraits<ModelData>::enumerate_sub_assets(path);
    case AssetType::Texture:
        return AssetTraits<TextureData>::enumerate_sub_assets(path);
    case AssetType::Shader:
        return AssetTraits<ShaderData>::enumerate_sub_assets(path);
    case AssetType::Animation:
        return AssetTraits<AnimationData>::enumerate_sub_assets(path);
    case AssetType::Count:
        break;
    }
    return std::vector<SubAssetDescriptor> {};
}

}
