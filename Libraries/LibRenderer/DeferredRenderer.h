/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <expected>
#include <memory>
#include <span>
#include <string>

#include <Common/Expected.h>
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
        i32 frames_in_flight = 2;
        RHI::TextureFormat render_target_format {};
        RHI::Device* device {};
        ResourceManager* resource_manager {};
    };

    static auto create(Configuration const& config) -> Common::Expected<std::unique_ptr<DeferredRenderer>>;

    void submit(SubmitInfo const& submit_info) const override;
    auto resize(u32 width, u32 height) -> Common::Expected<void> override;

    auto create_output_render_target(RHI::Texture const* output_texture) const -> Common::Expected<std::unique_ptr<RHI::RenderTarget>> override;
private:
    explicit DeferredRenderer(Configuration const& config);

    auto create_frame_resources(Configuration const& config) -> Common::Expected<void>;
    auto create_shadow_pass(Configuration const& config) -> Common::Expected<void>;
    auto create_geometry_pass(Configuration const& config) -> Common::Expected<void>;
    auto create_lighting_pass(Configuration const& config) -> Common::Expected<void>;
    auto create_gbuffer_textures(u32 width, u32 height) -> Common::Expected<GBuffer>;
    void bind_gbuffer_textures();
private:
    // --- Frame resources --- //
    RHI::Device* m_device {};
    std::unique_ptr<RHI::ResourceLayout> m_frame_resource_layout;
    std::vector<std::unique_ptr<RHI::ResourceSet>> m_frame_resource_sets;
    std::vector<std::unique_ptr<RHI::Buffer>> m_frame_uniform_buffers;
    std::unique_ptr<RHI::Sampler> m_frame_default_sampler;
    RHI::Pipeline::PushConstant m_model_push_constant {};

    // --- Shadow pass --- //
    std::unique_ptr<RHI::RenderPass> m_shadow_render_pass;
    std::unique_ptr<RHI::Pipeline> m_shadow_pipeline;
    std::unique_ptr<RHI::Pipeline> m_shadow_skinned_pipeline;
    std::unique_ptr<RHI::RenderTarget> m_shadow_render_target;
    std::unique_ptr<RHI::ResourceLayout> m_shadow_resource_layout;
    std::unique_ptr<RHI::ResourceSet> m_shadow_resource_set;
    std::unique_ptr<RHI::Texture> m_shadow_map;
    std::unique_ptr<RHI::Sampler> m_shadow_sampler;
    u32 m_shadow_map_size {};

    // --- Geometry pass --- //
    GBuffer m_gbuffer;
    std::unique_ptr<RHI::RenderPass> m_geometry_render_pass;
    std::unique_ptr<RHI::Pipeline> m_geometry_pipeline;
    std::unique_ptr<RHI::Pipeline> m_geometry_skinned_pipeline;

    // --- Lighting pass --- //
    std::unique_ptr<RHI::RenderPass> m_lighting_render_pass;
    std::unique_ptr<RHI::Pipeline> m_lighting_pipeline;
    std::unique_ptr<RHI::ResourceLayout> m_lighting_resource_layout;
    std::unique_ptr<RHI::ResourceSet> m_lighting_resource_set;
};

}
