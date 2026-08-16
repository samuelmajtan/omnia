/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibAsset/Pose.h>

namespace Asset {

Pose::Pose(Skeleton const& skeleton)
{
    reset_to_bind_pose(skeleton);
}

void Pose::reset_to_bind_pose(Skeleton const& skeleton)
{
    auto const& nodes = skeleton.nodes();

    m_translations.resize(nodes.size());
    m_rotations.resize(nodes.size());
    m_scales.resize(nodes.size());

    for (std::size_t index = 0; index < nodes.size(); ++index) {
        m_translations[index] = nodes[index].translation;
        m_rotations[index] = nodes[index].rotation;
        m_scales[index] = nodes[index].scale;
    }
}

auto Pose::size() const -> u64
{
    return m_translations.size();
}

auto Pose::translation(u32 node_index) const -> Math::Vec3f const&
{
    assert(node_index < m_translations.size());
    return m_translations[node_index];
}

auto Pose::rotation(u32 node_index) const -> Math::Quatf const&
{
    assert(node_index < m_rotations.size());
    return m_rotations[node_index];
}

auto Pose::scale(u32 node_index) const -> Math::Vec3f const&
{
    assert(node_index < m_scales.size());
    return m_scales[node_index];
}

void Pose::set_translation(u32 node_index, Math::Vec3f const& translation)
{
    assert(node_index < m_translations.size());
    m_translations[node_index] = translation;
}

void Pose::set_rotation(u32 node_index, Math::Quatf const& rotation)
{
    assert(node_index < m_rotations.size());
    m_rotations[node_index] = rotation;
}

void Pose::set_scale(u32 node_index, Math::Vec3f const& scale)
{
    assert(node_index < m_scales.size());
    m_scales[node_index] = scale;
}

auto Pose::local_matrix(u32 node_index) const -> Math::Mat4f
{
    assert(node_index < m_translations.size());
    return Math::Mat4f::from_trs(m_translations[node_index], m_rotations[node_index], m_scales[node_index]);
}

}
