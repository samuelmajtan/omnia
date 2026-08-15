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
    RHI::Buffer::Configuration const vertex_buffer_config {
        .size = configuration.vertices.size() * sizeof(Graphics::Vertex),
        .usage = RHI::BufferUsage::Vertex,
        .data = configuration.vertices.data()
    };
    std::unique_ptr<RHI::Buffer> vertex_buffer;
    TRY_ASSIGN(vertex_buffer, device->create_buffer(vertex_buffer_config));

    RHI::Buffer::Configuration const index_buffer_config {
        .size = configuration.indices.size() * sizeof(Graphics::Index),
        .usage = RHI::BufferUsage::Index,
        .data = configuration.indices.data()
    };
    std::unique_ptr<RHI::Buffer> index_buffer;
    TRY_ASSIGN(index_buffer, device->create_buffer(index_buffer_config));

    SubMesh submesh;
    submesh.m_vertex_buffer = std::move(vertex_buffer);
    submesh.m_index_buffer = std::move(index_buffer);
    submesh.m_index_count = configuration.indices.size();
    submesh.m_material_index = configuration.material_index;
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

}
