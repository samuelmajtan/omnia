/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

#include <LibMath/Math.h>

namespace Graphics {

struct Vertex {
    Math::Vec3f position;
    Math::Vec2f tex_coord;
    Math::Vec3f normal;
    Math::Vec4f tangent;
};

struct SkinnedVertex {
    Math::Vec3f position;
    Math::Vec2f tex_coord;
    Math::Vec3f normal;
    Math::Vec4f tangent;
    Math::Vec4u bone_indices;
    Math::Vec4f bone_weights;
};

inline constexpr u32 MAX_BONE_INFLUENCES = 4;

using Index = u32;

struct MaterialParameters {
    Math::Vec4f base_color { 1.0F, 1.0F, 1.0F, 1.0F };
    Math::Vec4f emissive_factor { 1.0F, 1.0F, 1.0F, 1.0F };
    f32 metallic_factor = 1.0F;
    f32 roughness_factor = 1.0F;
    f32 normal_scale = 1.0F;
    f32 occlusion_strength = 1.0F;
};

struct SkeletonNode {
    std::string name;
    i32 parent_index = -1;
    Math::Vec3f translation {};
    Math::Quatf rotation {};
    Math::Vec3f scale { 1.0F, 1.0F, 1.0F };
};

struct SkeletonData {
    std::vector<SkeletonNode> nodes;
    std::vector<u32> bone_nodes;
    std::vector<Math::Mat4f> inverse_bind_matrices;
};

struct SubMeshData {
    std::vector<Vertex> vertices;
    std::vector<SkinnedVertex> skinned_vertices;
    std::vector<Index> indices;
    u64 material_index;

    auto is_skinned() const -> bool
    {
        return !skinned_vertices.empty();
    }

    auto vertex_count() const -> u64
    {
        return is_skinned() ? skinned_vertices.size() : vertices.size();
    }
};

}
