/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <cassert>
#include <format>

#include <Common/Expected.h>
#include <LibRHI/Vulkan/VkRenderPass.h>
#include <LibRHI/Vulkan/VkRenderTarget.h>
#include <LibRHI/Vulkan/VkSwapchain.h>

namespace RHI {

auto VkRenderPass::create(const RHI::RenderPass::Configuration& config, RHI::VkDevice const* device) -> Common::Expected<std::unique_ptr<RHI::VkRenderPass>>
{
    std::unique_ptr<VkRenderPass> render_pass(new VkRenderPass(config, device));

    TRY(render_pass->create_render_pass());
    return render_pass;
}

VkRenderPass::VkRenderPass(Configuration const& config, RHI::VkDevice const* device)
    : m_config(config)
    , m_device(device)
{
    assert(m_device);
}

VkRenderPass::~VkRenderPass()
{
    if (m_handle != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device->handle(), m_handle, nullptr);
    }
}

auto VkRenderPass::handle() const -> ::VkRenderPass
{
    return m_handle;
}

void VkRenderPass::begin(CommandBuffer const* command_buffer, RenderTarget const* render_target) const
{
    auto const* vk_cmd = to_vk(command_buffer);
    auto const* vk_render_target = to_vk(render_target);

    VkRenderPassBeginInfo const render_pass_begin_info {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = m_handle,
        .framebuffer = vk_render_target->framebuffer(),
        .renderArea = {
            .offset = { 0, 0 },
            .extent = { vk_render_target->width(), vk_render_target->height() } },
        .clearValueCount = static_cast<u32>(m_clear_values.size()),
        .pClearValues = m_clear_values.data()
    };

    vkCmdBeginRenderPass(vk_cmd->handle(), &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void VkRenderPass::end(CommandBuffer const* command_buffer) const
{
    auto const* vk_cmd = to_vk(command_buffer);

    vkCmdEndRenderPass(vk_cmd->handle());
}

auto VkRenderPass::create_render_pass() -> Common::Expected<void>
{
    std::vector<VkAttachmentDescription> attachment_descriptions;
    std::vector<VkAttachmentReference> color_attachment_references;

    // clang-format off
    for (auto const& color_attachment : m_config.color_attachments) {
        VkAttachmentDescription const color_attachment_description {
            .flags = 0,
            .format = to_vk(color_attachment.format),
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = to_vk(color_attachment.load_op),
            .storeOp = to_vk(color_attachment.store_op),
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = to_vk(color_attachment.initial_layout),
            .finalLayout = to_vk(color_attachment.final_layout)
        };
        color_attachment_references.emplace_back(attachment_descriptions.size(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        attachment_descriptions.push_back(color_attachment_description);

        VkClearValue const clear_value {
            .color = {
                .float32 = {
                    color_attachment.clear_color.x,
                    color_attachment.clear_color.y,
                    color_attachment.clear_color.z,
                    color_attachment.clear_color.w
                }
            }
        };
        m_clear_values.push_back(clear_value);
    }
    // clang-format on

    VkAttachmentReference depth_attachment_reference {};
    if (m_config.depth_attachment.has_value()) {
        auto const& depth_attachment = m_config.depth_attachment.value();

        VkClearValue const clear_value {
            .color = {
                .float32 = {
                    depth_attachment.clear_color.x,
                    depth_attachment.clear_color.y,
                    depth_attachment.clear_color.z,
                    depth_attachment.clear_color.w } }
        };
        m_clear_values.push_back(clear_value);

        VkAttachmentDescription const depth_attachment_description {
            .flags = 0,
            .format = to_vk(depth_attachment.format),
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = to_vk(depth_attachment.load_op),
            .storeOp = to_vk(depth_attachment.store_op),
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = to_vk(depth_attachment.initial_layout),
            .finalLayout = to_vk(depth_attachment.final_layout)
        };
        depth_attachment_reference.attachment = static_cast<u32>(attachment_descriptions.size());
        depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachment_descriptions.push_back(depth_attachment_description);
    }

    VkSubpassDescription const subpass_description {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = static_cast<u32>(color_attachment_references.size()),
        .pColorAttachments = color_attachment_references.data(),
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = m_config.depth_attachment.has_value() ? &depth_attachment_reference : nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr
    };

    VkSubpassDependency const subpass_dependency {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0,
    };

    VkRenderPassCreateInfo const render_pass_create_info {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .attachmentCount = static_cast<u32>(attachment_descriptions.size()),
        .pAttachments = attachment_descriptions.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass_description,
        .dependencyCount = 1,
        .pDependencies = &subpass_dependency
    };

    if (auto result = vkCreateRenderPass(m_device->handle(), &render_pass_create_info, nullptr, &m_handle); result != VK_SUCCESS) {
        return OA_ERROR("Failed to create Vulkan render pass: {}", string_VkResult(result));
    }
    return {};
}

auto to_vk(LoadOp load_op) -> VkAttachmentLoadOp
{
    switch (load_op) {
    case LoadOp::Load:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case LoadOp::Clear:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case LoadOp::DontCare:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}

auto to_vk(StoreOp store_op) -> VkAttachmentStoreOp
{
    switch (store_op) {
    case StoreOp::Store:
        return VK_ATTACHMENT_STORE_OP_STORE;
    case StoreOp::DontCare:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
}

auto to_vk(ImageLayout layout) -> VkImageLayout
{
    switch (layout) {
    case ImageLayout::Undefined:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case ImageLayout::ShaderReadOnly:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case ImageLayout::ColorAttachment:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case ImageLayout::DepthReadOnly:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case ImageLayout::PresentSrc:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    return VK_IMAGE_LAYOUT_UNDEFINED;
}

auto to_vk(RenderPass const* render_pass) -> RHI::VkRenderPass const*
{
    return static_cast<RHI::VkRenderPass const*>(render_pass);
}

}
