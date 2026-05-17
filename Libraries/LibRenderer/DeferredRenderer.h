/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <expected>
#include <memory>
#include <string>

#include <Common/Types.h>
#include <LibRHI/Device.h>
#include <LibRenderer/Export.h>
#include <LibRenderer/Renderer.h>
#include <LibRenderer/ResourceManager.h>

namespace Renderer {

struct GBuffer {
    std::unique_ptr<RHI::Texture> normal;
    std::unique_ptr<RHI::Texture> albedo;
    std::unique_ptr<RHI::Texture> material;
    std::unique_ptr<RHI::Texture> depth;
    std::unique_ptr<RHI::Texture> emissive;
    std::unique_ptr<RHI::RenderTarget> render_target;
};

class RENDERER_API DeferredRenderer final : public Renderer {
public:
    struct Configuration {
        u32 render_target_width {};
        u32 render_target_height {};
        u32 shadow_map_size = 4096;
        RHI::TextureFormat render_target_format {};
        RHI::Device* device {};
        ResourceManager* resource_manager {};
    };

    static auto create(Configuration const& config) -> std::expected<std::unique_ptr<DeferredRenderer>, std::string>;

    void submit(SubmitInfo const& submit_info) const override;
    auto resize(u32 width, u32 height) -> std::expected<void, std::string> override;

    auto create_output_render_target(RHI::Texture const* output_texture) const -> std::expected<std::unique_ptr<RHI::RenderTarget>, std::string> override;
private:
    explicit DeferredRenderer(Configuration const& config);

    auto create_gbuffer_textures(u32 width, u32 height) -> std::expected<GBuffer, std::string>;
    auto create_resources(Configuration const& config) -> std::expected<void, std::string>;
    auto create_passes(Configuration const& config) -> std::expected<void, std::string>;
    auto create_pipelines(Configuration const& config) -> std::expected<void, std::string>;
private:
    // --- Lighting pass --- //
    std::unique_ptr<RHI::RenderPass> m_shadow_render_pass;
    std::unique_ptr<RHI::Pipeline> m_shadow_pipeline;
    std::unique_ptr<RHI::RenderTarget> m_shadow_render_target;
    std::unique_ptr<RHI::ResourceLayout> m_shadow_resource_layout;
    std::unique_ptr<RHI::ResourceSet> m_shadow_resource_set;
    std::unique_ptr<RHI::Texture> m_shadow_map;
    std::unique_ptr<RHI::Sampler> m_shadow_sampler;
    RHI::Shader const* m_shadow_vertex_shader {};
    u32 m_shadow_map_size {};

    // --- Geometry Pass --- //
    GBuffer m_gbuffer;
    std::unique_ptr<RHI::RenderPass> m_geometry_render_pass;
    std::unique_ptr<RHI::Pipeline> m_geometry_pipeline;
    RHI::Shader const* m_geometry_vertex_shader {};
    RHI::Shader const* m_geometry_fragment_shader {};

    // --- Lighting pass --- //
    std::unique_ptr<RHI::RenderPass> m_lighting_render_pass;
    std::unique_ptr<RHI::Pipeline> m_lighting_pipeline;
    std::unique_ptr<RHI::ResourceLayout> m_lighting_resource_layout;
    std::unique_ptr<RHI::ResourceSet> m_lighting_resource_set;
    RHI::Shader const* m_lighting_vertex_shader {};
    RHI::Shader const* m_lighting_fragment_shader {};

    // --- Resources --- //
    RHI::Device* m_device {};
    ResourceManager* m_resource_manager {};
    std::unique_ptr<RHI::ResourceLayout> m_frame_resource_layout;
    std::unique_ptr<RHI::ResourceSet> m_frame_resource_set;
    std::unique_ptr<RHI::Buffer> m_frame_uniform_buffer;
    std::unique_ptr<RHI::Sampler> m_frame_default_sampler;
    RHI::Pipeline::PushConstant m_model_push_constant {};
};

}
