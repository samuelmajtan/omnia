/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Common/Expected.h>
#include <Common/Noncopyable.h>
#include <LibRHI/Vulkan/VkDevice.h>

namespace RHI {

class VkStagingBuffer final {
    OA_MAKE_NONCOPYABLE(VkStagingBuffer);
    OA_MAKE_DEFAULT_CONSTRUCTIBLE(VkStagingBuffer);

public:
    static auto create(VkDevice const* device, u64 size) -> Common::Expected<VkStagingBuffer>;

    VkStagingBuffer(VkStagingBuffer&& other);
    auto operator=(VkStagingBuffer&& other) -> VkStagingBuffer&;

    ~VkStagingBuffer();

    auto handle() const -> ::VkBuffer;
    auto allocation() const -> VmaAllocation;
    auto allocation_info() const -> VmaAllocationInfo const&;
private:
    ::VkBuffer m_handle {};
    RHI::VkDevice const* m_device {};
    VmaAllocation m_allocation {};
    VmaAllocationInfo m_allocation_info {};
};

}
