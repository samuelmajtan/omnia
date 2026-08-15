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
#include <LibRHI/ResourceSet.h>
#include <LibRHI/Vulkan/VkDevice.h>

namespace RHI {

class VkResourceSet : public ResourceSet {
public:
    static auto create(Configuration const& config, RHI::VkDevice* device) -> Common::Expected<std::unique_ptr<VkResourceSet>>;

    ~VkResourceSet() override;

    auto handle() const -> VkDescriptorSet;

    void set_sampler(u32 binding, Sampler const* sampler) override;
    void set_texture(u32 binding, Texture const* texture) override;
    void set_depth_texture(u32 binding, Texture const* texture) override;
    void set_uniform_buffer(u32 binding, Buffer const* buffer) override;
private:
    VkResourceSet(Configuration const& config, RHI::VkDevice const* device);
private:
    VkDescriptorSet m_handle {};
    RHI::VkDevice const* m_device {};
};

auto to_vk(ResourceSet const* resource_set) -> VkResourceSet const*;

}
