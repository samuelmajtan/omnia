/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <expected>
#include <memory>
#include <string>

#include <Common/Expected.h>
#include <Common/Noncopyable.h>
#include <LibRHI/Texture.h>
#include <LibRHI/Vulkan/VkDevice.h>

namespace RHI {

class VkTexture final : public Texture {
public:
    static auto create_owned(Configuration const& config, RHI::VkDevice const* device) -> Common::Expected<std::unique_ptr<VkTexture>>;
    static auto create_borrowed(Configuration const& config, RHI::VkDevice const* device, VkImage image, VkImageView image_view) -> Common::Expected<std::unique_ptr<VkTexture>>;

    ~VkTexture() override;

    auto width() const -> u32 override;
    auto height() const -> u32 override;
    auto format() const -> TextureFormat override;
    auto image() const -> VkImage;
    auto image_view() const -> VkImageView;
private:
    VkTexture(Configuration const& config, RHI::VkDevice const* device);
private:
    u32 m_width {};
    u32 m_height {};
    TextureFormat m_format {};
    RHI::VkDevice const* m_device {};
    VkImage m_image {};
    VkImageView m_image_view {};
    VmaAllocation m_allocation {};
    bool m_owned = false;
};

auto to_graphics(VkFormat format) -> TextureFormat;
auto to_vk_aspect(TextureFormat format) -> VkImageAspectFlags;
auto to_vk(TextureUsage usage) -> VkImageUsageFlags;
auto to_vk(TextureFormat format) -> VkFormat;
auto to_vk(Texture const* texture) -> VkTexture const*;

}
