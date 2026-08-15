/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <cassert>
#include <format>

#include <Common/Expected.h>
#include <LibRHI/Vulkan/VkSampler.h>

namespace RHI {

auto VkSampler::create(Configuration const& config, RHI::VkDevice const* device) -> Common::Expected<std::unique_ptr<VkSampler>>
{
    std::unique_ptr<VkSampler> sampler(new VkSampler(device));

    VkSamplerCreateInfo const sampler_create_info {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = to_vk(config.mag_filter),
        .minFilter = to_vk(config.min_filter),
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = to_vk(config.address_mode.u),
        .addressModeV = to_vk(config.address_mode.v),
        .addressModeW = to_vk(config.address_mode.w),
        .mipLodBias = 0.0F,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 1.0F,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0F,
        .maxLod = 0.0F,
        .borderColor = to_vk(config.border_color),
        .unnormalizedCoordinates = VK_FALSE
    };

    if (auto result = vkCreateSampler(device->handle(), &sampler_create_info, nullptr, &sampler->m_handle); result != VK_SUCCESS) {
        return OA_ERROR("Failed to create Vulkan sampler: {}", string_VkResult(result));
    }

    return sampler;
}

VkSampler::VkSampler(RHI::VkDevice const* device)
    : m_device(device)
{
    assert(device != nullptr);
}

VkSampler::~VkSampler()
{
    if (m_handle != VK_NULL_HANDLE) {
        vkDestroySampler(m_device->handle(), m_handle, nullptr);
    }
}

auto VkSampler::handle() const -> ::VkSampler
{
    return m_handle;
}

auto to_vk(Sampler const* sampler) -> RHI::VkSampler const*
{
    return static_cast<RHI::VkSampler const*>(sampler);
}

auto to_vk(Filter filter) -> VkFilter
{
    switch (filter) {
    case Filter::Nearest:
        return VK_FILTER_NEAREST;
    case Filter::Linear:
        return VK_FILTER_LINEAR;
    }
}

auto to_vk(AddressMode address_mode) -> VkSamplerAddressMode
{
    switch (address_mode) {
    case AddressMode::Repeat:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case AddressMode::MirroredRepeat:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case AddressMode::ClampToEdge:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case AddressMode::ClampToBorder:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    }
}

auto to_vk(BorderColor border_color) -> VkBorderColor
{
    switch (border_color) {
    case BorderColor::TransparentBlack:
        return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    case BorderColor::OpaqueBlack:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    case BorderColor::OpaqueWhite:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    }
}

}
