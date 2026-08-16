/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string>

#include <Common/Types.h>
#include <LibAsset/Export.h>
#include <LibAsset/Pose.h>
#include <LibAsset/AnimationImporter.h>

namespace Asset {

struct AnimationSegment {
    std::size_t previous;
    std::size_t next;
    f32 alpha;
};

class ASSET_API AnimationClip final {
public:
    AnimationClip() = default;
    explicit AnimationClip(AnimationData data);

    auto name() const -> std::string const&;
    auto duration() const -> f32;
    auto channel_count() const -> u64;
    void sample(f32 time, Pose& pose) const;
private:
    auto segment_at(std::vector<Keyframe> const& keyframes, f32 time) const -> AnimationSegment;
private:
    AnimationData m_data;
};

}
