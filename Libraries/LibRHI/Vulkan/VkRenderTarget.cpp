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
#include <LibRHI/Vulkan/VkTexture.h>

namespace RHI {

auto VkRenderTarget::create(Configuration const& config, RHI::VkDevice const* device) -> Common::Expected<std::unique_ptr<VkRenderTarget>>
{
    std::unique_ptr<VkRenderTarget> render_target(new VkRenderTarget(config, device));

    std::vector<VkImageView> attachments;

    for (auto const* texture : config.textures) {
        assert(texture->width() == render_target->m_width);
        assert(texture->height() == render_target->m_height);

        auto* vk_texture = to_vk(texture);
        attachments.push_back(vk_texture->image_view());
    }

    if (config.depth_texture != nullptr) {
        auto* vk_texture = to_vk(config.depth_texture);
        attachments.push_back(vk_texture->image_view());
    }

    auto const* vk_render_pass = to_vk(config.render_pass);
    VkFramebufferCreateInfo const framebuffer_create_info {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderPass = vk_render_pass->handle(),
        .attachmentCount = static_cast<u32>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = render_target->m_width,
        .height = render_target->m_height,
        .layers = 1
    };

    if (auto result = vkCreateFramebuffer(device->handle(), &framebuffer_create_info, nullptr, &render_target->m_framebuffer); result != VK_SUCCESS) {
        return OA_ERROR("Failed to create framebuffer: {}", string_VkResult(result));
    }

    return render_target;
}

VkRenderTarget::VkRenderTarget(Configuration const& config, RHI::VkDevice const* device)
    : m_device(device)
    , m_width(config.width)
    , m_height(config.height)
{
    assert(m_device != nullptr);
}

VkRenderTarget::~VkRenderTarget()
{
    if (m_framebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(m_device->handle(), m_framebuffer, nullptr);
    }
}

auto VkRenderTarget::framebuffer() const -> VkFramebuffer
{
    return m_framebuffer;
}

auto VkRenderTarget::width() const -> u32
{
    return m_width;
}

auto VkRenderTarget::height() const -> u32
{
    return m_height;
}

auto to_vk(RenderTarget const* render_target) -> VkRenderTarget const*
{
    return static_cast<VkRenderTarget const*>(render_target);
}

}
