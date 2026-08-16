/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#define CGLTF_IMPLEMENTATION
#include <LibAsset/glTF.h>

#include <algorithm>
#include <cstring>
#include <format>

#include <Common/File.h>

namespace Asset::glTF {

auto parse(std::filesystem::path const& path) -> Common::Expected<Data>
{
    auto const file_data = TRY(File::read_all(path));

    cgltf_options const options {};
    cgltf_data* data = nullptr;
    if (cgltf_parse(&options, file_data.data(), file_data.size(), &data) != cgltf_result_success) {
        return OA_ERROR("Failed to parse glTF file '{}'", path.string());
    }
    return Data(data, &cgltf_free);
}

auto load(std::filesystem::path const& path) -> Common::Expected<Data>
{
    auto data = TRY(parse(path));

    cgltf_options const options {};
    if (cgltf_load_buffers(&options, data.get(), path.string().c_str()) != cgltf_result_success) {
        return OA_ERROR("Failed to load buffers for glTF file '{}'", path.string());
    }
    if (cgltf_validate(data.get()) != cgltf_result_success) {
        return OA_ERROR("Invalid glTF file '{}'", path.string());
    }
    return data;
}

auto local_transform(cgltf_node const& node) -> std::tuple<Math::Vec3f, Math::Quatf, Math::Vec3f>
{
    if (node.has_matrix) {
        Math::Mat4f matrix;
        std::memcpy(matrix.data(), node.matrix, sizeof(f32) * 16);
        return matrix.decompose();
    }

    return {
        node.has_translation ? Math::Vec3f(node.translation[0], node.translation[1], node.translation[2]) : Math::Vec3f {},
        node.has_rotation ? Math::Quatf(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]) : Math::Quatf::identity(),
        node.has_scale ? Math::Vec3f(node.scale[0], node.scale[1], node.scale[2]) : Math::Vec3f(1.0F, 1.0F, 1.0F)
    };
}

auto flatten_node_hierarchy(cgltf_data const* data) -> std::pair<std::vector<Graphics::SkeletonNode>, NodeIndices>
{
    std::vector<Graphics::SkeletonNode> nodes;
    NodeIndices indices;
    nodes.reserve(data->nodes_count);

    std::vector<std::pair<cgltf_node const*, i32>> pending;
    for (cgltf_size i = data->nodes_count; i-- > 0;) {
        if (data->nodes[i].parent == nullptr) {
            pending.emplace_back(&data->nodes[i], -1);
        }
    }

    while (!pending.empty()) {
        auto const [node, parent_index] = pending.back();
        pending.pop_back();

        auto const [translation, rotation, scale] = local_transform(*node);
        auto const index = static_cast<u32>(nodes.size());
        indices[node] = index;
        nodes.push_back({
            .name = node->name != nullptr ? node->name : std::format("Node_{}", index),
            .parent_index = parent_index,
            .translation = translation,
            .rotation = rotation,
            .scale = scale });

        for (cgltf_size i = node->children_count; i-- > 0;) {
            pending.emplace_back(node->children[i], static_cast<i32>(index));
        }
    }

    return { std::move(nodes), std::move(indices) };
}

auto clip_name(cgltf_animation const& animation, cgltf_size index) -> std::string
{
    auto name = animation.name != nullptr ? std::string(animation.name) : std::format("Animation_{}", index);
    std::ranges::replace(name, SUB_ASSET_SEPARATOR, '_');
    return name;
}

}
