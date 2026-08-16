/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <Common/Types.h>
#include <LibAsset/Export.h>
#include <LibGraphics/ModelTypes.h>

namespace Asset {

class ASSET_API Skeleton final {
public:
    Skeleton() = default;
    explicit Skeleton(Graphics::SkeletonData data);

    auto nodes() const -> std::vector<Graphics::SkeletonNode> const&;
    auto node_count() const -> u64;
    auto bone_count() const -> u64;
    auto bone_nodes() const -> std::vector<u32> const&;
    auto inverse_bind_matrices() const -> std::vector<Math::Mat4f> const&;

    auto find_node(std::string_view name) const -> std::optional<u32>;
private:
    Graphics::SkeletonData m_data;
    std::unordered_map<std::string, u32> m_nodes_by_name;
};

}
