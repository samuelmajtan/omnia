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
#include <LibRHI/Buffer.h>
#include <LibRHI/Vulkan/VkDevice.h>

namespace RHI {

class VkBuffer final : public Buffer {
public:
    static auto create(Configuration const& config, RHI::VkDevice const* device) -> Common::Expected<std::unique_ptr<VkBuffer>>;

    ~VkBuffer() override;

    void set_data(void const* data, u64 size) override;
    auto handle() const -> ::VkBuffer;
private:
    VkBuffer(Configuration const& config, RHI::VkDevice const* device);

    auto create_buffer() -> Common::Expected<void>;
    auto upload_data() -> Common::Expected<void>;
private:
    Configuration m_config {};
    RHI::VkDevice const* m_device {};
    ::VkBuffer m_handle {};
    VmaAllocation m_allocation {};
    VmaAllocationInfo m_allocation_info {};
};

auto to_vk(Buffer const* buffer) -> RHI::VkBuffer const*;
auto to_vk(BufferUsage usage) -> VkBufferUsageFlags;

}
