/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <cassert>
#include <format>

#include <Common/Expected.h>
#include <LibRHI/Vulkan/VkBuffer.h>
#include <LibRHI/Vulkan/VkCommandBuffer.h>
#include <LibRHI/Vulkan/VkResourceSet.h>
#include <LibRHI/Vulkan/VkShader.h>

namespace RHI {

auto VkCommandBuffer::create(VkCommandPool command_pool, const RHI::VkDevice* device) -> Common::Expected<VkCommandBuffer>
{
    RHI::VkCommandBuffer command_buffer;
    VkCommandBufferAllocateInfo const command_buffer_allocate_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    if (auto result = vkAllocateCommandBuffers(device->handle(), &command_buffer_allocate_info, &command_buffer.m_handle); result != VK_SUCCESS) {
        return OA_ERROR("Failed to allocate Vulkan command buffer: {}", string_VkResult(result));
    }
    return command_buffer;
}

auto VkCommandBuffer::handle() const -> ::VkCommandBuffer
{
    return m_handle;
}

void VkCommandBuffer::reset() const
{
    vkResetCommandBuffer(m_handle, 0);
}

void VkCommandBuffer::begin() const
{
    begin(0);
}

void VkCommandBuffer::begin(VkCommandBufferUsageFlags usage) const
{
    VkCommandBufferBeginInfo const command_buffer_begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = usage,
        .pInheritanceInfo = nullptr
    };
    vkBeginCommandBuffer(m_handle, &command_buffer_begin_info);
}

void VkCommandBuffer::end() const
{
    vkEndCommandBuffer(m_handle);
}

void VkCommandBuffer::transition_image_layout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout) const
{
    VkImageMemoryBarrier barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = 0,
        .dstAccessMask = 0,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1 }
    };

    VkPipelineStageFlags source_stage {};
    VkPipelineStageFlags destination_stage {};

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    vkCmdPipelineBarrier(m_handle, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VkCommandBuffer::begin_render_pass(RenderPass const* render_pass, RenderTarget const* render_target) const
{
    render_pass->begin(this, render_target);
}

void VkCommandBuffer::end_render_pass() const
{
    vkCmdEndRenderPass(m_handle);
}

void VkCommandBuffer::bind_pipeline(Pipeline const* pipeline)
{
    auto const* vk_pipeline = to_vk(pipeline);

    m_current_pipeline_layout = vk_pipeline->layout();
    vkCmdBindPipeline(m_handle, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->handle());
}

void VkCommandBuffer::bind_resource_set(u32 set_index, ResourceSet const* resource_set) const
{
    assert(m_current_pipeline_layout != VK_NULL_HANDLE && "Pipeline must be bound before binding resource sets.");

    auto* resource_set_handle = to_vk(resource_set)->handle();
    vkCmdBindDescriptorSets(m_handle, VK_PIPELINE_BIND_POINT_GRAPHICS, m_current_pipeline_layout, set_index, 1, &resource_set_handle, 0, nullptr);
}

void VkCommandBuffer::bind_vertex_buffer(Buffer const* vertex_buffer) const
{
    auto* vk_vertex_buffer = to_vk(vertex_buffer)->handle();
    VkDeviceSize const offset = 0;
    vkCmdBindVertexBuffers(m_handle, 0, 1, &vk_vertex_buffer, &offset);
}

void VkCommandBuffer::bind_index_buffer(Buffer const* index_buffer) const
{
    auto* vk_index_buffer = to_vk(index_buffer)->handle();
    vkCmdBindIndexBuffer(m_handle, vk_index_buffer, 0, VK_INDEX_TYPE_UINT32);
}

void VkCommandBuffer::draw(u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) const
{
    vkCmdDraw(m_handle, vertex_count, instance_count, first_vertex, first_instance);
}

void VkCommandBuffer::draw_indexed(u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance) const
{
    vkCmdDrawIndexed(m_handle, index_count, instance_count, first_index, vertex_offset, first_instance);
}

void VkCommandBuffer::push_constants(Pipeline::PushConstant const& push_constant, void const* data) const
{
    assert(m_current_pipeline_layout != VK_NULL_HANDLE && "Pipeline must be bound before pushing constants.");

    vkCmdPushConstants(m_handle, m_current_pipeline_layout, to_vk(push_constant.stage), push_constant.offset, push_constant.size, data);
}

void VkCommandBuffer::set_viewport(u32 x, u32 y, u32 width, u32 height) const
{
    VkViewport const viewport {
        .x = static_cast<f32>(x),
        .y = static_cast<f32>(y),
        .width = static_cast<f32>(width),
        .height = static_cast<f32>(height),
        .minDepth = 0.0F,
        .maxDepth = 1.0F
    };
    vkCmdSetViewport(m_handle, 0, 1, &viewport);
}

void VkCommandBuffer::set_scissor(u32 x, u32 y, u32 width, u32 height) const
{
    VkRect2D const scissor {
        .offset = { static_cast<i32>(x), static_cast<i32>(y) },
        .extent = { width, height }
    };
    vkCmdSetScissor(m_handle, 0, 1, &scissor);
}

auto to_vk(CommandBuffer const* command_buffer) -> VkCommandBuffer const*
{
    return static_cast<VkCommandBuffer const*>(command_buffer);
}

}
