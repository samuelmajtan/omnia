/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <expected>
#include <string>

#include <Common/Expected.h>
#include <LibRenderer/Export.h>
#include <LibAsset/Skeleton.h>
#include <LibRenderer/Material.h>
#include <LibRenderer/SubMesh.h>

namespace Renderer {

class RENDERER_API Model final {
    OA_MAKE_NONCOPYABLE(Model);
    OA_MAKE_DEFAULT_MOVABLE(Model);
    OA_MAKE_DEFAULT_DESTRUCTIBLE(Model);

public:
    struct Configuration {
        std::vector<SubMesh::Configuration> sub_meshes;
        std::vector<Material::Configuration> materials;
        std::optional<Graphics::SkeletonData> skeleton;
    };

    static auto create(Configuration const& configuration, RHI::Device* device) -> Common::Expected<std::unique_ptr<Model>>;

    auto sub_meshes() const -> std::vector<SubMesh> const&;
    auto materials() const -> std::vector<Material> const&;
    auto skeleton() const -> std::optional<Asset::Skeleton> const&;
private:
    Model() = default;

    auto create_sub_meshes(std::vector<SubMesh::Configuration> const& configurations, RHI::Device const* device) -> Common::Expected<void>;
    auto create_materials(std::vector<Material::Configuration> const& configurations, RHI::Device* device) -> Common::Expected<void>;
private:
    std::vector<SubMesh> m_sub_meshes;
    std::vector<Material> m_materials;
    std::optional<Asset::Skeleton> m_skeleton;
};

}
