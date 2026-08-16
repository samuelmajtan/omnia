/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <algorithm>

#include <Common/Expected.h>
#include <LibRenderer/AnimatedModel.h>
#include <LibRenderer/Log.h>

namespace Renderer {

AnimatedModel::AnimatedModel(Model const* model, std::vector<Asset::AnimationClip> animations)
    : m_model(model)
    , m_animations(std::move(animations))
    , m_player(model->skeleton().value())
{
}

auto AnimatedModel::create(Configuration config, RHI::Device* device) -> Common::Expected<std::unique_ptr<AnimatedModel>>
{
    assert(device != nullptr);
    assert(config.resource_layout != nullptr);

    if (config.model == nullptr) {
        return OA_ERROR("An animated model needs a model to animate");
    }
    if (!config.model->skeleton().has_value()) {
        return OA_ERROR("A model without a skeleton cannot be animated");
    }

    RHI::Buffer::Configuration const bone_buffer_config {
        .size = MAX_BONES * sizeof(Math::Mat4f),
        .usage = RHI::BufferUsage::Uniform
    };

    std::unique_ptr<AnimatedModel> animated_model(new AnimatedModel(config.model, std::move(config.animations)));
    animated_model->m_bone_buffers.reserve(config.frames_in_flight);
    animated_model->m_bone_resource_sets.reserve(config.frames_in_flight);

    for (u32 frame = 0; frame < config.frames_in_flight; ++frame) {
        animated_model->m_bone_buffers.push_back(TRY(device->create_buffer(bone_buffer_config)));
        animated_model->m_bone_resource_sets.push_back(TRY(device->create_resource_set({ .layout = config.resource_layout })));
        animated_model->m_bone_resource_sets.back()->set_uniform_buffer(0, animated_model->m_bone_buffers.back().get());
    }

    auto const bone_count = config.model->skeleton()->bone_count();
    if (bone_count > MAX_BONES) {
        OA_LOG_WARN(Log::Resources, "A skeleton has {} bones but the maximum holds {}, the rest are ignored", bone_count, MAX_BONES);
    }

    return animated_model;
}

auto AnimatedModel::play(std::string_view animation_name) -> Common::Expected<void>
{
    auto const* clip = find_animation(animation_name);
    if (clip == nullptr) {
        return OA_ERROR("'{}' is not one of the {} animations of this model", animation_name, m_animations.size());
    }

    m_player.set_clip(clip);
    return {};
}

void AnimatedModel::stop()
{
    m_player.set_clip(nullptr);
}

void AnimatedModel::set_looping(bool looping)
{
    m_player.set_looping(looping);
}

void AnimatedModel::set_speed(f32 speed)
{
    m_player.set_speed(speed);
}

void AnimatedModel::update(f32 delta_seconds, u32 frame_index)
{
    assert(frame_index < m_bone_buffers.size());

    m_player.update(delta_seconds);

    auto const bone_matrices = m_player.bone_matrices();
    auto const count = std::min<u64>(bone_matrices.size(), MAX_BONES);
    m_bone_buffers[frame_index]->set_data(bone_matrices.data(), count * sizeof(Math::Mat4f));
}

auto AnimatedModel::model() const -> Model const*
{
    return m_model;
}

auto AnimatedModel::animations() const -> std::vector<Asset::AnimationClip> const&
{
    return m_animations;
}

auto AnimatedModel::find_animation(std::string_view name) const -> Asset::AnimationClip const*
{
    auto const it = std::ranges::find(m_animations, name, &Asset::AnimationClip::name);
    return it != m_animations.end() ? &*it : nullptr;
}

auto AnimatedModel::player() const -> Asset::AnimationPlayer const&
{
    return m_player;
}

auto AnimatedModel::bone_resource_set(u32 frame_index) const -> RHI::ResourceSet const*
{
    assert(frame_index < m_bone_resource_sets.size());
    return m_bone_resource_sets[frame_index].get();
}

}
