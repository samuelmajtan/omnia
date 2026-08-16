/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Common/Expected.h>
#include <LibRenderer/SubMesh.h>

namespace Renderer {

auto SubMesh::create(Configuration const& configuration, RHI::Device const* device) -> Common::Expected<SubMesh>
{
    auto const skinned = configuration.is_skinned();
    RHI::Buffer::Configuration const vertex_buffer_config {
        .size = skinned ? configuration.skinned_vertices.size() * sizeof(Graphics::SkinnedVertex)
                        : configuration.vertices.size() * sizeof(Graphics::Vertex),
        .usage = RHI::BufferUsage::Vertex,
        .data = skinned ? static_cast<void const*>(configuration.skinned_vertices.data())
                        : static_cast<void const*>(configuration.vertices.data())
    };

    RHI::Buffer::Configuration const index_buffer_config {
        .size = configuration.indices.size() * sizeof(Graphics::Index),
        .usage = RHI::BufferUsage::Index,
        .data = configuration.indices.data()
    };

    SubMesh submesh;
    submesh.m_vertex_buffer = TRY(device->create_buffer(vertex_buffer_config));
    submesh.m_index_buffer = TRY(device->create_buffer(index_buffer_config));
    submesh.m_index_count = configuration.indices.size();
    submesh.m_material_index = configuration.material_index;
    submesh.m_is_skinned = skinned;
    return submesh;
}

auto SubMesh::vertex_buffer() const -> RHI::Buffer const*
{
    return m_vertex_buffer.get();
}

auto SubMesh::index_buffer() const -> RHI::Buffer const*
{
    return m_index_buffer.get();
}

auto SubMesh::index_count() const -> u64
{
    return m_index_count;
}

auto SubMesh::material_index() const -> u64
{
    return m_material_index;
}

auto SubMesh::is_skinned() const -> bool
{
    return m_is_skinned;
}

}
