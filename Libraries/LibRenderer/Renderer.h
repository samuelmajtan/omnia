/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <span>

#include <Common/Expected.h>
#include <LibMath/Math.h>
#include <LibRHI/Device.h>
#include <LibRenderer/Light.h>

namespace Renderer {

struct RenderItem {
    RHI::Buffer const* vertex_buffer;
    RHI::Buffer const* index_buffer;
    RHI::ResourceSet const* material_resource_set;
    u64 index_count;
    Math::Mat4f model_matrix;
};

struct FrameData {
    Math::Mat4f projection;
    Math::Mat4f view;
    Math::Vec4f camera_position;
    DirectionalLight directional_light;
};

class Renderer {
public:
    struct SubmitInfo {
        FrameData frame_data;
        RHI::RenderTarget const* output_render_target {};
        RHI::CommandBuffer* command_buffer {};
        std::span<RenderItem const> render_items;
    };

    virtual void submit(SubmitInfo const& submit_info) const = 0;
    virtual auto resize(u32 width, u32 height) -> Common::Expected<void> = 0;
    virtual auto create_output_render_target(RHI::Texture const* output_texture) const -> Common::Expected<std::unique_ptr<RHI::RenderTarget>> = 0;
};

}
