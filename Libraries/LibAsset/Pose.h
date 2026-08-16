/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <vector>

#include <Common/Types.h>
#include <LibAsset/Export.h>
#include <LibAsset/Skeleton.h>
#include <LibMath/Math.h>

namespace Asset {

class ASSET_API Pose final {
public:
    Pose() = default;
    explicit Pose(Skeleton const& skeleton);

    void reset_to_bind_pose(Skeleton const& skeleton);

    auto size() const -> u64;
    auto translation(u32 node_index) const -> Math::Vec3f const&;
    auto rotation(u32 node_index) const -> Math::Quatf const&;
    auto scale(u32 node_index) const -> Math::Vec3f const&;
    void set_translation(u32 node_index, Math::Vec3f const& translation);
    void set_rotation(u32 node_index, Math::Quatf const& rotation);
    void set_scale(u32 node_index, Math::Vec3f const& scale);
    auto local_matrix(u32 node_index) const -> Math::Mat4f;
private:
    std::vector<Math::Vec3f> m_translations;
    std::vector<Math::Quatf> m_rotations;
    std::vector<Math::Vec3f> m_scales;
};

}
