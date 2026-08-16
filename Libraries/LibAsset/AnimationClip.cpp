/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <algorithm>

#include <LibAsset/AnimationClip.h>

namespace Asset {

AnimationClip::AnimationClip(AnimationData data)
    : m_data(std::move(data))
{
}

auto AnimationClip::name() const -> std::string const&
{
    return m_data.name;
}

auto AnimationClip::duration() const -> f32
{
    return m_data.duration;
}

auto AnimationClip::channel_count() const -> u64
{
    return m_data.channels.size();
}

void AnimationClip::sample(f32 time, Pose& pose) const
{
    for (auto const& channel : m_data.channels) {
        if (channel.keyframes.empty() || channel.target_node >= pose.size()) {
            continue;
        }

        auto const segment = segment_at(channel.keyframes, time);
        auto const stepped = channel.interpolation == AnimationInterpolation::Step;
        auto const alpha = stepped ? 0.0F : segment.alpha;

        switch (channel.target_property) {
        case AnimationPath::Translation:
            pose.set_translation(channel.target_node, lerp(channel.keyframes[segment.previous].vector(), channel.keyframes[segment.next].vector(), alpha));
            break;
        case AnimationPath::Rotation:
            pose.set_rotation(channel.target_node, Math::slerp(channel.keyframes[segment.previous].quaternion(), channel.keyframes[segment.next].quaternion(), alpha));
            break;
        case AnimationPath::Scale:
            pose.set_scale(channel.target_node, lerp(channel.keyframes[segment.previous].vector(), channel.keyframes[segment.next].vector(), alpha));
            break;
        case AnimationPath::Count:
            break;
        }
    }
}

auto AnimationClip::segment_at(std::vector<Keyframe> const& keyframes, f32 time) const -> AnimationSegment
{
    if (time <= keyframes.front().time) {
        return { .previous = 0, .next = 0, .alpha = 0.0F };
    }
    if (time >= keyframes.back().time) {
        auto const last = keyframes.size() - 1;
        return { .previous = last, .next = last, .alpha = 0.0F };
    }

    auto const upper = std::ranges::upper_bound(keyframes, time, {}, &Keyframe::time);
    auto const next = static_cast<std::size_t>(std::distance(keyframes.begin(), upper));
    auto const previous = next - 1;

    auto const span = keyframes[next].time - keyframes[previous].time;
    auto const alpha = span > 0.0F ? (time - keyframes[previous].time) / span : 0.0F;
    return { .previous = previous, .next = next, .alpha = alpha };
}


}
