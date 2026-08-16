/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <algorithm>

#include <Common/Expected.h>
#include <Common/File.h>
#include <Common/Time.h>
#include <LibAsset/AnimationImporter.h>
#include <LibAsset/Log.h>
#include <LibAsset/glTF.h>

namespace Asset {

static auto to_path(cgltf_animation_path_type type) -> std::optional<AnimationPath>
{
    switch (type) {
    case cgltf_animation_path_type_translation:
        return AnimationPath::Translation;
    case cgltf_animation_path_type_rotation:
        return AnimationPath::Rotation;
    case cgltf_animation_path_type_scale:
        return AnimationPath::Scale;
    default:
        return std::nullopt;
    }
}

static auto to_interpolation(cgltf_interpolation_type type) -> std::optional<AnimationInterpolation>
{
    switch (type) {
    case cgltf_interpolation_type_step:
        return AnimationInterpolation::Step;
    case cgltf_interpolation_type_linear:
        return AnimationInterpolation::Linear;
    default:
        return std::nullopt;
    }
}

auto AnimationImporter::import(ImportContext const& context) -> Common::Expected<AnimationData>
{
    if (!context.sub_asset.has_value()) {
        return OA_ERROR("Animations are sub-assets, so '{}' has to name a clip inside the file", context.path.string());
    }

    auto const gltf = TRY(glTF::load(context.path));
    auto const* data = gltf.get();
    auto const& wanted = context.sub_asset.value();

    Time::Stopwatch const stopwatch;
    auto const [nodes, node_indices] = glTF::flatten_node_hierarchy(data);

    for (cgltf_size index = 0; index < data->animations_count; ++index) {
        auto const& animation = data->animations[index];
        if (glTF::clip_name(animation, index) != wanted) {
            continue;
        }

        AnimationData clip { .name = wanted, .duration = 0.0F, .channels = {} };
        clip.channels.reserve(animation.channels_count);

        for (cgltf_size i = 0; i < animation.channels_count; ++i) {
            auto const& channel = animation.channels[i];
            if (channel.target_node == nullptr || channel.sampler == nullptr) {
                continue;
            }

            auto const path = to_path(channel.target_path);
            if (!path.has_value()) {
                OA_LOG_TRACE(Log::Animation, "{}: dropping a channel of '{}' targeting an unsupported property", context.path.filename().string(), wanted);
                continue;
            }

            auto const node = node_indices.find(channel.target_node);
            if (node == node_indices.end()) {
                OA_LOG_WARN(Log::Animation, "{}: a channel of '{}' targets a node outside the hierarchy, dropping it", context.path.filename().string(), wanted);
                continue;
            }

            auto const interpolation = to_interpolation(channel.sampler->interpolation);
            if (!interpolation.has_value()) {
                return OA_ERROR("Clip '{}' in '{}' uses CUBICSPLINE interpolation, which is not supported yet", wanted, context.path.string());
            }

            auto const* input = channel.sampler->input;
            auto const* output = channel.sampler->output;
            if (input == nullptr || output == nullptr) {
                continue;
            }

            auto const components = components_for(path.value());
            if (output->count != input->count) {
                return OA_ERROR("Clip '{}' in '{}' has a channel with {} keyframe times but {} values", wanted, context.path.string(), input->count, output->count);
            }

            AnimationChannel imported {
                .target_node = node->second,
                .target_property = path.value(),
                .interpolation = interpolation.value(),
                .keyframes = std::vector<Keyframe>(input->count)
            };

            for (cgltf_size key = 0; key < input->count; ++key) {
                cgltf_accessor_read_float(input, key, &imported.keyframes[key].time, 1);
                cgltf_accessor_read_float(output, key, &imported.keyframes[key].value.x, components);
            }

            auto const ascending = std::ranges::is_sorted(imported.keyframes, {}, &Keyframe::time);
            if (!ascending) {
                return OA_ERROR("Clip '{}' in '{}' has a channel whose keyframe times are not ascending", wanted, context.path.string());
            }
            if (!imported.keyframes.empty()) {
                clip.duration = std::max(clip.duration, imported.keyframes.back().time);
            }

            clip.channels.push_back(std::move(imported));
        }

        OA_LOG_DEBUG(Log::Animation, "Imported '{}' from {}: {} channels, {:.2f}s, {:.1f}ms",
            wanted, context.path.filename().string(), clip.channels.size(), clip.duration, stopwatch.elapsed_milliseconds());
        return clip;
    }

    return OA_ERROR("'{}' has no animation named '{}'", context.path.string(), wanted);
}

auto AnimationImporter::source_hash(ImportContext const& context) -> Common::Expected<u64>
{
    return File::hash_file(context.path);
}

auto AnimationImporter::supported_extensions() -> std::vector<std::string>
{
    return {};
}

}
