/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include <Common/Expected.h>
#include <Common/Types.h>
#include <LibAsset/AnimationClip.h>
#include <LibAsset/AnimationPlayer.h>
#include <LibRHI/Device.h>
#include <LibRenderer/Export.h>
#include <LibRenderer/Model.h>

namespace Renderer {

class RENDERER_API AnimatedModel final {
    OA_MAKE_NONCOPYABLE(AnimatedModel);
    OA_MAKE_DEFAULT_MOVABLE(AnimatedModel);
    OA_MAKE_DEFAULT_DESTRUCTIBLE(AnimatedModel);

public:
    static constexpr u32 MAX_BONES = 128;

    struct Configuration {
        Model const* model {};
        std::vector<Asset::AnimationClip> animations;
        u32 frames_in_flight = 2;
        RHI::ResourceLayout const* resource_layout {};
    };

    static auto create(Configuration config, RHI::Device* device) -> Common::Expected<std::unique_ptr<AnimatedModel>>;

    auto play(std::string_view animation_name) -> Common::Expected<void>;
    void stop();
    void set_looping(bool looping);
    void set_speed(f32 speed);
    void update(f32 delta_seconds, u32 frame_index);

    auto model() const -> Model const*;
    auto is_looping() const -> bool;
    auto animations() const -> std::vector<Asset::AnimationClip> const&;
    auto find_animation(std::string_view name) const -> Asset::AnimationClip const*;
    auto player() const -> Asset::AnimationPlayer const&;
    auto bone_resource_set(u32 frame_index) const -> RHI::ResourceSet const*;
private:
    AnimatedModel(Model const* model, std::vector<Asset::AnimationClip> animations);

    Model const* m_model {};
    std::vector<Asset::AnimationClip> m_animations;
    Asset::AnimationPlayer m_player;
    std::vector<std::unique_ptr<RHI::Buffer>> m_bone_buffers;
    std::vector<std::unique_ptr<RHI::ResourceSet>> m_bone_resource_sets;
};

}
