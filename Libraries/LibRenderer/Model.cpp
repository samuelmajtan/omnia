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
        m_sub_meshes.push_back(TRY(SubMesh::create(sub_mesh_configuration, device)));
    }
    return {};
}

auto Model::create_materials(std::vector<Material::Configuration> const& configurations, RHI::Device* device) -> Common::Expected<void>
{
    m_materials.reserve(configurations.size());
    for (auto const& material_configuration : configurations) {
        m_materials.push_back(TRY(Material::create(material_configuration, device)));
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
