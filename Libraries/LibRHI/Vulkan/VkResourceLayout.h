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
#include <LibRHI/ResourceLayout.h>
#include <LibRHI/Vulkan/VkDevice.h>

namespace RHI {

class VkResourceLayout final : public ResourceLayout {
public:
    static auto create(Configuration const& config, RHI::VkDevice const* device) -> Common::Expected<std::unique_ptr<VkResourceLayout>>;

    ~VkResourceLayout() override;

    auto handle() const -> VkDescriptorSetLayout;
private:
    VkResourceLayout(RHI::VkDevice const* device);
private:
    RHI::VkDevice const* m_device {};
    VkDescriptorSetLayout m_handle {};
};

auto to_vk(ResourceLayout const* layout) -> VkResourceLayout const*;
auto to_vk(ResourceType type) -> VkDescriptorType;

}