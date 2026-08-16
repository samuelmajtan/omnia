/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibAsset/Skeleton.h>

namespace Asset {

Skeleton::Skeleton(Graphics::SkeletonData data)
    : m_data(std::move(data))
{
    m_nodes_by_name.reserve(m_data.nodes.size());
    for (u32 index = 0; index < m_data.nodes.size(); ++index) {
        m_nodes_by_name.emplace(m_data.nodes[index].name, index);
    }
}

auto Skeleton::nodes() const -> std::vector<Graphics::SkeletonNode> const&
{
    return m_data.nodes;
}

auto Skeleton::node_count() const -> u64
{
    return m_data.nodes.size();
}

auto Skeleton::bone_count() const -> u64
{
    return m_data.bone_nodes.size();
}

auto Skeleton::bone_nodes() const -> std::vector<u32> const&
{
    return m_data.bone_nodes;
}

auto Skeleton::inverse_bind_matrices() const -> std::vector<Math::Mat4f> const&
{
    return m_data.inverse_bind_matrices;
}

auto Skeleton::find_node(std::string_view name) const -> std::optional<u32>
{
    auto const it = m_nodes_by_name.find(std::string(name));
    if (it == m_nodes_by_name.end()) {
        return std::nullopt;
    }
    return it->second;
}

}
