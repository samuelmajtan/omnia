/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <algorithm>

#include <Common/Expected.h>
#include <Common/Time.h>
#include <LibRenderer/DeferredRenderer.h>
#include <LibRenderer/Log.h>

namespace Renderer {

auto DeferredRenderer::create(Configuration const& config) -> Common::Expected<std::unique_ptr<DeferredRenderer>>
{
    std::unique_ptr<DeferredRenderer> renderer(new DeferredRenderer(config));
    Time::Stopwatch const stopwatch;

    return renderer->create_frame_resources(config)
        .and_then([&]() {
            OA_LOG_TRACE(Log::Deferred, "Frame resources created");
            return renderer->create_shadow_pass(config);
        })
        .and_then([&]() {
            OA_LOG_TRACE(Log::Deferred, "Shadow pass created");
            return renderer->create_geometry_pass(config);
        })
        .and_then([&]() {
            OA_LOG_TRACE(Log::Deferred, "Geometry pass created");
            return renderer->create_lighting_pass(config);
        })
        .transform([&]() {
            OA_LOG_DEBUG(Log::Deferred, "Deferred renderer ready at {}x{} in {:.1f}ms",
                config.render_target_width, config.render_target_height, stopwatch.elapsed_milliseconds());
            return std::move(renderer);
        });
}

DeferredRenderer::DeferredRenderer(Configuration const& config)
    : m_device(config.device)
    , m_shadow_map_size(config.shadow_map_size)
{
    assert(m_device);
    assert(config.resource_manager);
}

void DeferredRenderer::submit(SubmitInfo const& submit_info) const
{
    auto* cmd = submit_info.command_buffer;
    auto const* render_target = submit_info.output_render_target;

    assert(submit_info.frame_index < m_frame_uniform_buffers.size());
    auto const& uniform_buffer = m_frame_uniform_buffers[submit_info.frame_index];
    auto const& resource_set = m_frame_resource_sets[submit_info.frame_index];

    uniform_buffer->set_data(&submit_info.frame_data, sizeof(submit_info.frame_data));

    auto const draw = [&](RenderItem const& render_item, SubMesh const& sub_mesh) {
        cmd->bind_vertex_buffer(sub_mesh.vertex_buffer());
        cmd->bind_index_buffer(sub_mesh.index_buffer());
        cmd->push_constants(m_model_push_constant, &render_item.model_matrix);
        cmd->draw_indexed(sub_mesh.index_count(), 1, 0, 0, 0);
    };

    auto const for_each_sub_mesh = [&](bool skinned, auto const& body) {
        for (auto const& render_item : submit_info.render_items) {
            assert(render_item.model != nullptr);
            for (auto const& sub_mesh : render_item.model->sub_meshes()) {
                if (sub_mesh.is_skinned() != skinned) {
                    continue;
                }
                assert(!skinned || render_item.bone_resource_set != nullptr);
                body(render_item, sub_mesh);
            }
        }
    };

    // --- Shadow Pass --- //
    cmd->begin_render_pass(m_shadow_render_pass.get(), m_shadow_render_target.get());
    {
        cmd->set_viewport(0, 0, m_shadow_map_size, m_shadow_map_size);
        cmd->set_scissor(0, 0, m_shadow_map_size, m_shadow_map_size);

        cmd->bind_pipeline(m_shadow_pipeline.get());
        cmd->bind_resource_set(0, resource_set.get());
        for_each_sub_mesh(false, draw);

        cmd->bind_pipeline(m_shadow_skinned_pipeline.get());
        cmd->bind_resource_set(0, resource_set.get());
        for_each_sub_mesh(true, [&](RenderItem const& render_item, SubMesh const& sub_mesh) {
            cmd->bind_resource_set(2, render_item.bone_resource_set);
            draw(render_item, sub_mesh);
        });
    }
    cmd->end_render_pass();

    // --- Geometry Pass --- //
    cmd->begin_render_pass(m_geometry_render_pass.get(), m_gbuffer.render_target.get());
    {
        cmd->set_viewport(0, 0, m_gbuffer.albedo->width(), m_gbuffer.albedo->height());
        cmd->set_scissor(0, 0, m_gbuffer.albedo->width(), m_gbuffer.albedo->height());

        auto const bind_material = [&](RenderItem const& render_item, SubMesh const& sub_mesh) {
            auto const& materials = render_item.model->materials();
            cmd->bind_resource_set(1, materials[sub_mesh.material_index()].resource_set());
        };

        cmd->bind_pipeline(m_geometry_pipeline.get());
        cmd->bind_resource_set(0, resource_set.get());
        for_each_sub_mesh(false, [&](RenderItem const& render_item, SubMesh const& sub_mesh) {
            bind_material(render_item, sub_mesh);
            draw(render_item, sub_mesh);
        });

        cmd->bind_pipeline(m_geometry_skinned_pipeline.get());
        cmd->bind_resource_set(0, resource_set.get());
        for_each_sub_mesh(true, [&](RenderItem const& render_item, SubMesh const& sub_mesh) {
            bind_material(render_item, sub_mesh);
            cmd->bind_resource_set(2, render_item.bone_resource_set);
            draw(render_item, sub_mesh);
        });
    }
    cmd->end_render_pass();

    // --- Lighting Pass --- //
    cmd->begin_render_pass(m_lighting_render_pass.get(), render_target);
    {
        cmd->bind_pipeline(m_lighting_pipeline.get());
        cmd->set_viewport(0, 0, render_target->width(), render_target->height());
        cmd->set_scissor(0, 0, render_target->width(), render_target->height());
        cmd->bind_resource_set(0, resource_set.get());
        cmd->bind_resource_set(1, m_lighting_resource_set.get());
        cmd->bind_resource_set(2, m_shadow_resource_set.get());
        cmd->draw(3, 1, 0, 0);
    }
    cmd->end_render_pass();
}

auto DeferredRenderer::resize(u32 width, u32 height) -> Common::Expected<void>
{
    m_gbuffer = TRY(create_gbuffer_textures(width, height));
    bind_gbuffer_textures();
    return {};
}

void DeferredRenderer::bind_gbuffer_textures()
{
    m_lighting_resource_set->set_texture(0, m_gbuffer.normal.get());
    m_lighting_resource_set->set_texture(1, m_gbuffer.albedo.get());
    m_lighting_resource_set->set_texture(2, m_gbuffer.material.get());
    m_lighting_resource_set->set_texture(3, m_gbuffer.emissive.get());
    m_lighting_resource_set->set_depth_texture(4, m_gbuffer.depth.get());
}

auto DeferredRenderer::create_gbuffer_textures(u32 width, u32 height) -> Common::Expected<GBuffer>
{
    GBuffer gbuffer {};
    RHI::Texture::Configuration gbuffer_texture_config {
        .width = width,
        .height = height,
        .format = RHI::TextureFormat::R16G16B16A16_SFLOAT,
        .usage = RHI::TextureUsage::ColorAttachment | RHI::TextureUsage::Sampled,
        .data = {}
    };
    gbuffer.normal = TRY(m_device->create_texture(gbuffer_texture_config));

    gbuffer_texture_config.format = RHI::TextureFormat::R8G8B8A8_UNORM;
    gbuffer.albedo = TRY(m_device->create_texture(gbuffer_texture_config));
    gbuffer.material = TRY(m_device->create_texture(gbuffer_texture_config));
    gbuffer.emissive = TRY(m_device->create_texture(gbuffer_texture_config));

    RHI::Texture::Configuration const depth_texture_config {
        .width = width,
        .height = height,
        .format = RHI::TextureFormat::D32_SFLOAT,
        .usage = RHI::TextureUsage::DepthStencil | RHI::TextureUsage::Sampled,
        .data = {}
    };
    gbuffer.depth = TRY(m_device->create_texture(depth_texture_config));

    RHI::RenderTarget::Configuration const geometry_render_target_config {
        .render_pass = m_geometry_render_pass.get(),
        .textures = {
            gbuffer.normal.get(),
            gbuffer.albedo.get(),
            gbuffer.material.get(),
            gbuffer.emissive.get()
        },
        .depth_texture = gbuffer.depth.get(),
        .width = width,
        .height = height
    };
    gbuffer.render_target = TRY(m_device->create_render_target(geometry_render_target_config));

    return gbuffer;
}

auto DeferredRenderer::create_frame_resources(Configuration const& config) -> Common::Expected<void>
{
    RHI::Buffer::Configuration const frame_uniform_buffer_config {
        .size = sizeof(FrameData),
        .usage = RHI::BufferUsage::Uniform
    };
    m_frame_uniform_buffers.reserve(config.frames_in_flight);
    for (i32 i = 0; i < config.frames_in_flight; ++i) {
        m_frame_uniform_buffers.push_back(TRY(config.device->create_buffer(frame_uniform_buffer_config)));
    }

    RHI::ResourceLayout::Configuration const frame_resource_layout_config {
        .bindings = {
            {
                .binding = 0,
                .type = RHI::ResourceType::Sampler,
                .stage = Graphics::ShaderStageMask::Fragment
            },
            {
                .binding = 1,
                .type = RHI::ResourceType::UniformBuffer,
                .stage = Graphics::ShaderStageMask::Vertex | Graphics::ShaderStageMask::Fragment
            }
        }
    };
    m_frame_resource_layout = TRY(config.device->create_resource_layout(frame_resource_layout_config));

    RHI::Sampler::Configuration const frame_default_sampler_config {
        .mag_filter = RHI::Filter::Linear,
        .min_filter = RHI::Filter::Linear,
        .address_mode = {
            .u = RHI::AddressMode::Repeat,
            .v = RHI::AddressMode::Repeat,
            .w = RHI::AddressMode::Repeat
        }
    };
    m_frame_default_sampler = TRY(config.device->create_sampler(frame_default_sampler_config));

    RHI::ResourceSet::Configuration const frame_resource_set_config {
        .layout = m_frame_resource_layout.get(),
    };
    m_frame_resource_sets.reserve(config.frames_in_flight);
    for (i32 i = 0; i < config.frames_in_flight; ++i) {
        m_frame_resource_sets.push_back(TRY(config.device->create_resource_set(frame_resource_set_config)));
        m_frame_resource_sets.back()->set_sampler(0, m_frame_default_sampler.get());
        m_frame_resource_sets.back()->set_uniform_buffer(1, m_frame_uniform_buffers[i].get());
    }

    m_model_push_constant = {
        .size = sizeof(Math::Mat4f),
        .offset = 0,
        .stage = Graphics::ShaderStageMask::Vertex
    };

    return {};
}

auto DeferredRenderer::create_shadow_pass(Configuration const& config) -> Common::Expected<void>
{
    RHI::RenderPass::Configuration const shadow_render_pass_config {
        .color_attachments = {},
        .depth_attachment = RHI::RenderPass::Attachment {
            .format = RHI::TextureFormat::D32_SFLOAT,
            .load_op = RHI::LoadOp::Clear,
            .store_op = RHI::StoreOp::Store,
            .clear_color = { 1.0F, 0.0F, 0.0F, 0.0F },
            .initial_layout = RHI::ImageLayout::Undefined,
            .final_layout = RHI::ImageLayout::DepthReadOnly
        }
    };
    m_shadow_render_pass = TRY(config.device->create_render_pass(shadow_render_pass_config));

    RHI::Texture::Configuration const shadow_map_config {
        .width = m_shadow_map_size,
        .height = m_shadow_map_size,
        .format = RHI::TextureFormat::D32_SFLOAT,
        .usage = RHI::TextureUsage::DepthStencil | RHI::TextureUsage::Sampled,
        .data = {}
    };
    m_shadow_map = TRY(config.device->create_texture(shadow_map_config));

    RHI::RenderTarget::Configuration const shadow_render_target_config {
        .render_pass = m_shadow_render_pass.get(),
        .textures = {},
        .depth_texture = m_shadow_map.get(),
        .width = m_shadow_map_size,
        .height = m_shadow_map_size
    };
    m_shadow_render_target = TRY(config.device->create_render_target(shadow_render_target_config));

    RHI::Sampler::Configuration const shadow_sampler_config {
        .mag_filter = RHI::Filter::Linear,
        .min_filter = RHI::Filter::Linear,
        .address_mode = {
            .u = RHI::AddressMode::ClampToBorder,
            .v = RHI::AddressMode::ClampToBorder,
            .w = RHI::AddressMode::ClampToBorder
        },
        .border_color = RHI::BorderColor::OpaqueWhite
    };
    m_shadow_sampler = TRY(config.device->create_sampler(shadow_sampler_config));

    RHI::ResourceLayout::Configuration const shadow_resource_layout_config {
        .bindings = {
            { .binding = 0, .type = RHI::ResourceType::Sampler, .stage = Graphics::ShaderStageMask::Fragment },
            { .binding = 1, .type = RHI::ResourceType::Texture, .stage = Graphics::ShaderStageMask::Fragment }
        }
    };
    m_shadow_resource_layout = TRY(config.device->create_resource_layout(shadow_resource_layout_config));
    m_shadow_resource_set = TRY(config.device->create_resource_set({ .layout = m_shadow_resource_layout.get() }));
    m_shadow_resource_set->set_sampler(0, m_shadow_sampler.get());
    m_shadow_resource_set->set_depth_texture(1, m_shadow_map.get());

    RHI::Pipeline::VertexBinding const vertex_binding_config {
        .stride = sizeof(Graphics::Vertex),
        .attributes = {
            { .location = 0, .offset = offsetof(Graphics::Vertex, position), .format = RHI::AttributeFormat::Float32Vec3 }
        }
    };

    RHI::Pipeline::Configuration const pipeline_config {
        .vertex_shader = TRY(config.resource_manager->load_shader("Shaders/ShadowPass.vs")),
        .fragment_shader = nullptr,
        .rasterization = {
            .cull_mode = RHI::CullMode::Back,
            .front_face = RHI::FrontFace::CounterClockwise,
            .polygon_mode = RHI::PolygonMode::Fill
        },
        .depth = {
            .test_enable = true,
            .write_enable = true,
            .compare_op = RHI::CompareOp::Less
        },
        .render_pass = m_shadow_render_pass.get(),
        .vertex_binding = vertex_binding_config,
        .color_blend_attachments = {},
        .resource_layouts = { m_frame_resource_layout.get() },
        .push_constants = { m_model_push_constant }
    };
    m_shadow_pipeline = TRY(config.device->create_pipeline(pipeline_config));

    RHI::Pipeline::VertexBinding const skinned_vertex_binding_config {
        .stride = sizeof(Graphics::SkinnedVertex),
        .attributes = {
            { .location = 0, .offset = offsetof(Graphics::SkinnedVertex, position), .format = RHI::AttributeFormat::Float32Vec3 },
            { .location = 4, .offset = offsetof(Graphics::SkinnedVertex, bone_indices), .format = RHI::AttributeFormat::Uint32Vec4 },
            { .location = 5, .offset = offsetof(Graphics::SkinnedVertex, bone_weights), .format = RHI::AttributeFormat::Float32Vec4 }
        }
    };

    auto skinned_pipeline_config = pipeline_config;
    skinned_pipeline_config.vertex_shader = TRY(config.resource_manager->load_shader("Shaders/ShadowPassSkinned.vs"));
    skinned_pipeline_config.vertex_binding = skinned_vertex_binding_config;
    skinned_pipeline_config.resource_layouts = { m_frame_resource_layout.get(), config.resource_manager->material_resource_layout(), config.resource_manager->bone_resource_layout() };
    m_shadow_skinned_pipeline = TRY(config.device->create_pipeline(skinned_pipeline_config));

    return {};
}

auto DeferredRenderer::create_geometry_pass(Configuration const& config) -> Common::Expected<void>
{
    constexpr RHI::RenderPass::Attachment color_attachment_config {
        .format = RHI::TextureFormat::R8G8B8A8_UNORM,
        .load_op = RHI::LoadOp::Clear,
        .store_op = RHI::StoreOp::Store,
        .clear_color = { 0.0F, 0.0F, 0.0F, 0.0F },
        .initial_layout = RHI::ImageLayout::Undefined,
        .final_layout = RHI::ImageLayout::ShaderReadOnly
    };

    RHI::RenderPass::Configuration const render_pass_config {
        .color_attachments = {
            {
                .format = RHI::TextureFormat::R16G16B16A16_SFLOAT,
                .load_op = RHI::LoadOp::Clear,
                .store_op = RHI::StoreOp::Store,
                .clear_color = { 0.0F, 0.0F, 0.0F, 0.0F },
                .initial_layout = RHI::ImageLayout::Undefined,
                .final_layout = RHI::ImageLayout::ShaderReadOnly
            },
            color_attachment_config, // Albedo
            color_attachment_config, // Material
            color_attachment_config  // Emissive
        },
        .depth_attachment = RHI::RenderPass::Attachment {
            .format = RHI::TextureFormat::D32_SFLOAT,
            .load_op = RHI::LoadOp::Clear,
            .store_op = RHI::StoreOp::Store,
            .clear_color = { 1.0F, 0.0F, 0.0F, 0.0F },
            .initial_layout = RHI::ImageLayout::Undefined,
            .final_layout = RHI::ImageLayout::DepthReadOnly
        }
    };
    m_geometry_render_pass = TRY(config.device->create_render_pass(render_pass_config));

    m_gbuffer = TRY(create_gbuffer_textures(config.render_target_width, config.render_target_height));

    RHI::Pipeline::VertexBinding const vertex_binding_config {
        .stride = sizeof(Graphics::Vertex),
        .attributes = {
            { .location = 0, .offset = offsetof(Graphics::Vertex, position), .format = RHI::AttributeFormat::Float32Vec3 },
            { .location = 1, .offset = offsetof(Graphics::Vertex, tex_coord), .format = RHI::AttributeFormat::Float32Vec2 },
            { .location = 2, .offset = offsetof(Graphics::Vertex, normal), .format = RHI::AttributeFormat::Float32Vec3 },
            { .location = 3, .offset = offsetof(Graphics::Vertex, tangent), .format = RHI::AttributeFormat::Float32Vec4 }
        }
    };

    RHI::Pipeline::Configuration const pipeline_config {
        .vertex_shader = TRY(config.resource_manager->load_shader("Shaders/GeometryPass.vs")),
        .fragment_shader = TRY(config.resource_manager->load_shader("Shaders/GeometryPass.fs")),
        .rasterization = {
            .cull_mode = RHI::CullMode::Back,
            .front_face = RHI::FrontFace::CounterClockwise,
            .polygon_mode = RHI::PolygonMode::Fill
        },
        .depth = {
            .test_enable = true,
            .write_enable = true,
            .compare_op = RHI::CompareOp::Less
        },
        .render_pass = m_geometry_render_pass.get(),
        .vertex_binding = vertex_binding_config,
        .color_blend_attachments = { {}, {}, {}, {} },
        .resource_layouts = { m_frame_resource_layout.get(), config.resource_manager->material_resource_layout() },
        .push_constants = { m_model_push_constant }
    };
    m_geometry_pipeline = TRY(config.device->create_pipeline(pipeline_config));

    RHI::Pipeline::VertexBinding const skinned_vertex_binding_config {
        .stride = sizeof(Graphics::SkinnedVertex),
        .attributes = {
            { .location = 0, .offset = offsetof(Graphics::SkinnedVertex, position), .format = RHI::AttributeFormat::Float32Vec3 },
            { .location = 1, .offset = offsetof(Graphics::SkinnedVertex, tex_coord), .format = RHI::AttributeFormat::Float32Vec2 },
            { .location = 2, .offset = offsetof(Graphics::SkinnedVertex, normal), .format = RHI::AttributeFormat::Float32Vec3 },
            { .location = 3, .offset = offsetof(Graphics::SkinnedVertex, tangent), .format = RHI::AttributeFormat::Float32Vec4 },
            { .location = 4, .offset = offsetof(Graphics::SkinnedVertex, bone_indices), .format = RHI::AttributeFormat::Uint32Vec4 },
            { .location = 5, .offset = offsetof(Graphics::SkinnedVertex, bone_weights), .format = RHI::AttributeFormat::Float32Vec4 }
        }
    };

    auto skinned_pipeline_config = pipeline_config;
    skinned_pipeline_config.vertex_shader = TRY(config.resource_manager->load_shader("Shaders/GeometryPassSkinned.vs"));
    skinned_pipeline_config.vertex_binding = skinned_vertex_binding_config;
    skinned_pipeline_config.resource_layouts = { m_frame_resource_layout.get(), config.resource_manager->material_resource_layout(), config.resource_manager->bone_resource_layout() };
    m_geometry_skinned_pipeline = TRY(config.device->create_pipeline(skinned_pipeline_config));

    return {};
}

auto DeferredRenderer::create_lighting_pass(Configuration const& config) -> Common::Expected<void>
{
    RHI::RenderPass::Configuration const render_pass_config {
        .color_attachments = {
            {
                .format = config.render_target_format,
                .load_op = RHI::LoadOp::Clear,
                .store_op = RHI::StoreOp::Store,
                .clear_color = { 0.0F, 0.0F, 0.0F, 1.0F },
                .initial_layout = RHI::ImageLayout::Undefined,
                .final_layout = RHI::ImageLayout::PresentSrc
            }
        },
        .depth_attachment = std::nullopt
    };
    m_lighting_render_pass = TRY(config.device->create_render_pass(render_pass_config));

    RHI::ResourceLayout::Configuration const resource_layout_config {
        .bindings = {
            { .binding = 0, .type = RHI::ResourceType::Texture, .stage = Graphics::ShaderStageMask::Fragment }, // Normal
            { .binding = 1, .type = RHI::ResourceType::Texture, .stage = Graphics::ShaderStageMask::Fragment }, // Albedo
            { .binding = 2, .type = RHI::ResourceType::Texture, .stage = Graphics::ShaderStageMask::Fragment }, // Material
            { .binding = 3, .type = RHI::ResourceType::Texture, .stage = Graphics::ShaderStageMask::Fragment }, // Emissive
            { .binding = 4, .type = RHI::ResourceType::Texture, .stage = Graphics::ShaderStageMask::Fragment }, // Depth
        }
    };
    m_lighting_resource_layout = TRY(config.device->create_resource_layout(resource_layout_config));
    m_lighting_resource_set = TRY(config.device->create_resource_set({ .layout = m_lighting_resource_layout.get() }));
    bind_gbuffer_textures();

    RHI::Pipeline::Configuration const pipeline_config {
        .vertex_shader = TRY(config.resource_manager->load_shader("Shaders/LightingPass.vs")),
        .fragment_shader = TRY(config.resource_manager->load_shader("Shaders/LightingPass.fs")),
        .rasterization = {
            .cull_mode = RHI::CullMode::None,
            .front_face = RHI::FrontFace::CounterClockwise,
            .polygon_mode = RHI::PolygonMode::Fill
        },
        .depth = {
            .test_enable = false,
            .write_enable = false
        },
        .render_pass = m_lighting_render_pass.get(),
        .color_blend_attachments = { {} },
        .resource_layouts = { m_frame_resource_layout.get(), m_lighting_resource_layout.get(), m_shadow_resource_layout.get() },
    };
    m_lighting_pipeline = TRY(config.device->create_pipeline(pipeline_config));

    return {};
}

auto DeferredRenderer::create_output_render_target(RHI::Texture const* output_texture) const -> Common::Expected<std::unique_ptr<RHI::RenderTarget>>
{
    RHI::RenderTarget::Configuration const render_target_config {
        .render_pass = m_lighting_render_pass.get(),
        .textures = { output_texture },
        .depth_texture = nullptr,
        .width = output_texture->width(),
        .height = output_texture->height()
    };
    return m_device->create_render_target(render_target_config);
}

}
