/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Common/Expected.h>
#include <LibRenderer/Material.h>

namespace Renderer {

auto Material::create(Configuration const& configuration, RHI::Device* device) -> Common::Expected<Material>
{
    assert(configuration.resource_layout != nullptr);
    assert(configuration.albedo_texture != nullptr);

    Material material;
    material.m_parameters = configuration.parameters;

    auto resource_set_configuration = RHI::ResourceSet::Configuration {
        .layout = configuration.resource_layout,
    };
    TRY_ASSIGN(material.m_resource_set, device->create_resource_set(resource_set_configuration));

    auto uniform_buffer_configuration = RHI::Buffer::Configuration {
        .size = sizeof(Graphics::MaterialParameters),
        .usage = RHI::BufferUsage::Uniform,
        .data = &material.m_parameters
    };
    TRY_ASSIGN(material.m_uniform_buffer, device->create_buffer(uniform_buffer_configuration));

    material.m_resource_set->set_uniform_buffer(0, material.m_uniform_buffer.get());
    material.m_resource_set->set_texture(1, configuration.albedo_texture);
    material.m_resource_set->set_texture(2, configuration.metallic_roughness_texture);
    material.m_resource_set->set_texture(3, configuration.normal_texture);
    material.m_resource_set->set_texture(4, configuration.occlusion_texture);
    material.m_resource_set->set_texture(5, configuration.emissive_texture);

    return material;
}

auto Material::resource_set() const -> RHI::ResourceSet const*
{
    return m_resource_set.get();
}

}
