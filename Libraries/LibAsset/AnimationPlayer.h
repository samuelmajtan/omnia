/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <span>

#include <Common/Types.h>
#include <LibAsset/AnimationClip.h>
#include <LibAsset/Export.h>
#include <LibAsset/Pose.h>
#include <LibAsset/Skeleton.h>

namespace Asset {

class ASSET_API AnimationPlayer final {
public:
    explicit AnimationPlayer(Skeleton const& skeleton);

    void set_clip(AnimationClip const* clip);
    void set_looping(bool looping);
    void set_speed(f32 speed);
    void set_time(f32 seconds);

    void update(f32 delta_seconds);

    auto time() const -> f32;
    auto is_looping() const -> bool;
    auto has_finished() const -> bool;
    auto bone_matrices() const -> std::span<Math::Mat4f const>;
private:
    void evaluate();

    Skeleton const* m_skeleton;
    AnimationClip const* m_clip {};
    Pose m_pose;
    std::vector<Math::Mat4f> m_global_transforms;
    std::vector<Math::Mat4f> m_bone_matrices;
    f32 m_time {};
    f32 m_speed = 1.0F;
    bool m_looping = true;
};

}
