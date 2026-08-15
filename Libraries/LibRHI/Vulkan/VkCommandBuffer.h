/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Common/Expected.h>
#include <Common/Noncopyable.h>
#include <LibRHI/CommandBuffer.h>
#include <LibRHI/Vulkan/VkPipeline.h>

namespace RHI {

class VkDevice;

class VkCommandBuffer final : public CommandBuffer {
    OA_MAKE_NONCOPYABLE(VkCommandBuffer);
    OA_MAKE_DEFAULT_MOVABLE(VkCommandBuffer);
    OA_MAKE_DEFAULT_CONSTRUCTIBLE(VkCommandBuffer);

public:
    ~VkCommandBuffer() override = default;

    static auto create(VkCommandPool command_pool, RHI::VkDevice const* device) -> Common::Expected<VkCommandBuffer>;

    auto handle() const -> ::VkCommandBuffer;
    void reset() const override;

    void begin() const override;
    void begin(VkCommandBufferUsageFlags usage) const;
    void end() const override;

    void transition_image_layout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout) const;

    void begin_render_pass(RenderPass const* render_pass, RenderTarget const* render_target) const override;
    void end_render_pass() const override;

    void bind_pipeline(Pipeline const* pipeline) override;
    void bind_resource_set(u32 set_index, ResourceSet const* resource_set) const override;
    void bind_vertex_buffer(Buffer const* vertex_buffer) const override;
    void bind_index_buffer(Buffer const* index_buffer) const override;

    void draw(u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) const override;
    void draw_indexed(u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance) const override;

    void push_constants(Pipeline::PushConstant const& push_constant, void const* data) const override;

    void set_viewport(u32 x, u32 y, u32 width, u32 height) const override;
    void set_scissor(u32 x, u32 y, u32 width, u32 height) const override;
private:
    ::VkCommandBuffer m_handle {};
    ::VkPipelineLayout m_current_pipeline_layout {};
};

auto to_vk(CommandBuffer const* command_buffer) -> VkCommandBuffer const*;

}
