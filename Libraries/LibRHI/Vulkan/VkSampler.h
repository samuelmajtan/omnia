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
#include <LibRHI/Sampler.h>
#include <LibRHI/Vulkan/VkDevice.h>

namespace RHI {

class VkSampler final : public Sampler {
public:
    static auto create(Configuration const& config, RHI::VkDevice const* device) -> Common::Expected<std::unique_ptr<VkSampler>>;

    ~VkSampler() override;

    auto handle() const -> ::VkSampler;
private:
    VkSampler(RHI::VkDevice const* device);
private:
    ::VkSampler m_handle {};
    RHI::VkDevice const* m_device {};
};

auto to_vk(Sampler const* sampler) -> RHI::VkSampler const*;
auto to_vk(Filter filter) -> VkFilter;
auto to_vk(AddressMode address_mode) -> VkSamplerAddressMode;
auto to_vk(BorderColor border_color) -> VkBorderColor;

}
