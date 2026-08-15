/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <cassert>
#include <format>

#include <Common/Expected.h>
#include <LibRHI/Vulkan/VkStagingBuffer.h>
#include <LibRHI/Vulkan/VkTexture.h>

namespace RHI {

auto VkTexture::create_owned(Configuration const& config, RHI::VkDevice const* device) -> Common::Expected<std::unique_ptr<VkTexture>>
{
    std::unique_ptr<VkTexture> texture(new VkTexture(config, device));
    texture->m_owned = true;

    VkImageCreateInfo const image_create_info {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = to_vk(config.format),
        .extent = {
            .width = config.width,
            .height = config.height,
            .depth = 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = to_vk(config.usage),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VmaAllocationCreateInfo const allocation_create_info {
        .flags = 0,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .preferredFlags = 0,
        .memoryTypeBits = 0,
        .pool = nullptr,
        .pUserData = nullptr,
        .priority = 0.0F,
    };

    if (auto result = vmaCreateImage(device->allocator(), &image_create_info, &allocation_create_info, &texture->m_image, &texture->m_allocation, nullptr); result != VK_SUCCESS) {
        return OA_ERROR("Failed to create Vulkan image: {}", string_VkResult(result));
    }

    VkImageViewCreateInfo const image_view_create_info {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = texture->m_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = to_vk(config.format),
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = { .aspectMask = to_vk_aspect(config.format), .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 }
    };
    if (auto result = vkCreateImageView(device->handle(), &image_view_create_info, nullptr, &texture->m_image_view); result != VK_SUCCESS) {
        return OA_ERROR("Failed to create Vulkan image view: {}", string_VkResult(result));
    }

    if (!config.data.empty()) {
        auto const staging_buffer = TRY(VkStagingBuffer::create(device, config.data.size()));
        std::memcpy(staging_buffer.allocation_info().pMappedData, config.data.data(), config.data.size());

        VkBufferImageCopy const copy_region {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1 },
            .imageOffset = { 0, 0, 0 },
            .imageExtent = { config.width, config.height, 1 }
        };

        auto& cmd = device->graphics_command_buffer();
        cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        cmd.transition_image_layout(texture->m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        vkCmdCopyBufferToImage(cmd.handle(), staging_buffer.handle(), texture->m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
        cmd.transition_image_layout(texture->m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        device->submit_graphics(cmd);
    }

    return texture;
}

auto VkTexture::create_borrowed(Configuration const& config, const RHI::VkDevice* device, VkImage image, VkImageView image_view) -> Common::Expected<std::unique_ptr<VkTexture>>
{
    std::unique_ptr<VkTexture> texture(new VkTexture(config, device));
    texture->m_image = image;
    texture->m_image_view = image_view;
    return texture;
}

VkTexture::VkTexture(const RHI::Texture::Configuration& config, const RHI::VkDevice* device)
    : m_width(config.width)
    , m_height(config.height)
    , m_format(config.format)
    , m_device(device)
{
    assert(device);
}

VkTexture::~VkTexture()
{
    if (m_owned && m_image != VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE) {
        vmaDestroyImage(m_device->allocator(), m_image, m_allocation);
    }
    if (m_image_view != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device->handle(), m_image_view, nullptr);
    }
}

auto VkTexture::width() const -> u32
{
    return m_width;
}

auto VkTexture::height() const -> u32
{
    return m_height;
}

auto VkTexture::format() const -> TextureFormat
{
    return m_format;
}

auto VkTexture::image() const -> VkImage
{
    return m_image;
}

auto VkTexture::image_view() const -> VkImageView
{
    return m_image_view;
}

auto to_graphics(VkFormat format) -> TextureFormat
{
    switch (format) {
        using enum TextureFormat;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return R16G16B16A16_SFLOAT;
    case VK_FORMAT_R8G8B8A8_SRGB:
        return R8G8B8A8_SRGB;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return R8G8B8A8_UNORM;
    case VK_FORMAT_D32_SFLOAT:
        return D32_SFLOAT;
    default:
        return Unknown;
    }
}

auto to_vk_aspect(TextureFormat format) -> VkImageAspectFlags
{
    switch (format) {
        using enum TextureFormat;
    case R16G16B16A16_SFLOAT:
    case R8G8B8A8_SRGB:
    case R8G8B8A8_UNORM:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    case D32_SFLOAT:
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    default:
        return 0;
    }
}

auto to_vk(TextureUsage usage) -> VkImageUsageFlags
{
    using enum TextureUsage;

    VkImageUsageFlags flags {};
    if (any(usage & Sampled)) {
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (any(usage & ColorAttachment)) {
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (any(usage & DepthStencil)) {
        flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    return flags;
}

auto to_vk(TextureFormat format) -> VkFormat
{
    switch (format) {
        using enum TextureFormat;
    case R16G16B16A16_SFLOAT:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case R8G8B8A8_SRGB:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case R8G8B8A8_UNORM:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case D32_SFLOAT:
        return VK_FORMAT_D32_SFLOAT;
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

auto to_vk(Texture const* texture) -> VkTexture const*
{
    return static_cast<VkTexture const*>(texture);
}

}
