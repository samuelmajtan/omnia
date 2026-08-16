/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <cgltf.h>
#include <filesystem>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <Common/Expected.h>
#include <LibAsset/Asset.h>
#include <LibGraphics/ModelTypes.h>
#include <LibMath/Math.h>

namespace Asset::glTF {

using Data = std::unique_ptr<cgltf_data, void (*)(cgltf_data*)>;
using NodeIndices = std::unordered_map<cgltf_node const*, u32>;

auto parse(std::filesystem::path const& path) -> Common::Expected<Data>;
auto load(std::filesystem::path const& path) -> Common::Expected<Data>;

auto local_transform(cgltf_node const& node) -> std::tuple<Math::Vec3f, Math::Quatf, Math::Vec3f>;
auto flatten_node_hierarchy(cgltf_data const* data) -> std::pair<std::vector<Graphics::SkeletonNode>, NodeIndices>;
auto clip_name(cgltf_animation const& animation, cgltf_size index) -> std::string;

}
