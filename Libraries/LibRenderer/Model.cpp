/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Common/Expected.h>
#include <LibRenderer/Model.h>

namespace Renderer {

auto Model::create(Configuration const& configuration, RHI::Device* device) -> Common::Expected<std::unique_ptr<Model>>
{
    assert(device != nullptr);

    std::unique_ptr<Model> model(new Model);
    return model->create_sub_meshes(configuration.sub_meshes, device)
        .and_then([&]() {
            return model->create_materials(configuration.materials, device);
        })
        .transform([&]() {
            return std::move(model);
        });
}

auto Model::create_sub_meshes(std::vector<SubMesh::Configuration> const& configurations, RHI::Device const* device) -> Common::Expected<void>
{
    m_sub_meshes.reserve(configurations.size());
    for (auto const& sub_mesh_configuration : configurations) {
        auto sub_mesh = SubMesh::create(sub_mesh_configuration, device);
        if (!sub_mesh.has_value()) {
            return std::unexpected(std::move(sub_mesh).error());
        }
        m_sub_meshes.push_back(std::move(sub_mesh).value());
    }
    return {};
}

auto Model::create_materials(std::vector<Material::Configuration> const& configurations, RHI::Device* device) -> Common::Expected<void>
{
    m_materials.reserve(configurations.size());
    for (auto const& material_configuration : configurations) {
        auto material = Material::create(material_configuration, device);
        if (!material.has_value()) {
            return std::unexpected(std::move(material).error());
        }
        m_materials.push_back(std::move(material).value());
    }
    return {};
}

auto Model::sub_meshes() const -> std::vector<SubMesh> const&
{
    return m_sub_meshes;
}

auto Model::materials() const -> std::vector<Material> const&
{
    return m_materials;
}

}
