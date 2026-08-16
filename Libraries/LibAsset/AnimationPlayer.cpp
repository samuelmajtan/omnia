/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <algorithm>
#include <cmath>

#include <LibAsset/AnimationPlayer.h>

namespace Asset {

AnimationPlayer::AnimationPlayer(Skeleton const& skeleton)
    : m_skeleton(&skeleton)
    , m_pose(skeleton)
{
    m_global_transforms.resize(skeleton.node_count());
    m_bone_matrices.assign(skeleton.bone_count(), Math::Mat4f::identity());
    evaluate();
}

void AnimationPlayer::set_clip(AnimationClip const* clip)
{
    m_clip = clip;
    m_time = 0.0F;
    evaluate();
}

void AnimationPlayer::set_looping(bool looping)
{
    m_looping = looping;
}

void AnimationPlayer::set_speed(f32 speed)
{
    m_speed = speed;
}

void AnimationPlayer::set_time(f32 seconds)
{
    m_time = seconds;
    evaluate();
}

void AnimationPlayer::update(f32 delta_seconds)
{
    if (m_clip == nullptr) {
        return;
    }

    m_time += delta_seconds * m_speed;

    auto const duration = m_clip->duration();
    if (duration > 0.0F) {
        if (m_looping) {
            m_time = std::fmod(m_time, duration);
            if (m_time < 0.0F) {
                m_time += duration;
            }
        } else {
            m_time = std::clamp(m_time, 0.0F, duration);
        }
    } else {
        m_time = 0.0F;
    }

    evaluate();
}

void AnimationPlayer::evaluate()
{
    m_pose.reset_to_bind_pose(*m_skeleton);
    if (m_clip != nullptr) {
        m_clip->sample(m_time, m_pose);
    }

    auto const& nodes = m_skeleton->nodes();
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        auto const local = m_pose.local_matrix(static_cast<u32>(index));
        auto const parent = nodes[index].parent_index;
        m_global_transforms[index] = parent < 0 ? local : m_global_transforms[parent] * local;
    }

    auto const& bone_nodes = m_skeleton->bone_nodes();
    auto const& inverse_bind_matrices = m_skeleton->inverse_bind_matrices();
    for (std::size_t bone = 0; bone < bone_nodes.size(); ++bone) {
        m_bone_matrices[bone] = m_global_transforms[bone_nodes[bone]] * inverse_bind_matrices[bone];
    }
}

auto AnimationPlayer::time() const -> f32
{
    return m_time;
}

auto AnimationPlayer::is_looping() const -> bool
{
    return m_looping;
}

auto AnimationPlayer::has_finished() const -> bool
{
    return !m_looping && m_clip != nullptr && m_time >= m_clip->duration();
}

auto AnimationPlayer::bone_matrices() const -> std::span<Math::Mat4f const>
{
    return m_bone_matrices;
}

}
