/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Common/Expected.h>
#include <LibAsset/Export.h>
#include <LibAsset/Importer.h>
#include <LibMath/Math.h>

namespace Asset {

enum class AnimationPath : u8 {
    Translation = 0,
    Rotation,
    Scale,
    Count
};

enum class AnimationInterpolation : u8 {
    Step = 0,
    Linear,
    Count
};

inline constexpr auto components_for(AnimationPath path) -> u32
{
    return path == AnimationPath::Rotation ? 4 : 3;
}

struct Keyframe {
    f32 time {};
    Math::Vec4f value {};

    auto vector() const -> Math::Vec3f
    {
        return value.xyz();
    }

    auto quaternion() const -> Math::Quatf
    {
        return { value.x, value.y, value.z, value.w };
    }
};

struct AnimationChannel {
    u32 target_node {};
    AnimationPath target_property {};
    AnimationInterpolation interpolation {};
    std::vector<Keyframe> keyframes;
};

struct AnimationData {
    std::string name;
    f32 duration {};
    std::vector<AnimationChannel> channels;
};

class ASSET_API AnimationImporter final {
public:
    static constexpr u32 VERSION = 2;

public:
    static auto import(ImportContext const& context) -> Common::Expected<AnimationData>;
    static auto source_hash(ImportContext const& context) -> Common::Expected<u64>;
    static auto supported_extensions() -> std::vector<std::string>;
};

}
