/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <format>

#include <Common/Expected.h>
#include <LibRHI/Vulkan/VkStagingBuffer.h>

namespace RHI {

auto VkStagingBuffer::create(const RHI::VkDevice* device, u64 size) -> Common::Expected<VkStagingBuffer>
{
    VkStagingBuffer staging_buffer {};
    staging_buffer.m_device = device;

    VkBufferCreateInfo const buffer_create_info {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    VmaAllocationCreateInfo const allocation_create_info {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .preferredFlags = 0,
        .memoryTypeBits = 0,
        .pool = nullptr,
        .pUserData = nullptr,
        .priority = 0.0F,
    };

    if (auto result = vmaCreateBuffer(staging_buffer.m_device->allocator(), &buffer_create_info, &allocation_create_info, &staging_buffer.m_handle, &staging_buffer.m_allocation, &staging_buffer.m_allocation_info); result != VK_SUCCESS) {
        return OA_ERROR("Failed to create Vulkan staging buffer: {}", string_VkResult(result));
    }

    return staging_buffer;
}

VkStagingBuffer::VkStagingBuffer(VkStagingBuffer&& other)
    : m_handle(std::exchange(other.m_handle, nullptr))
    , m_device(std::exchange(other.m_device, nullptr))
    , m_allocation(std::exchange(other.m_allocation, {}))
    , m_allocation_info(std::exchange(other.m_allocation_info, {}))
{
}

auto VkStagingBuffer::operator=(VkStagingBuffer&& other) -> VkStagingBuffer&
{
    if (this != &other) {
        if (m_handle != VK_NULL_HANDLE) {
            vmaDestroyBuffer(m_device->allocator(), m_handle, m_allocation);
        }
        m_handle = std::exchange(other.m_handle, nullptr);
        m_device = std::exchange(other.m_device, nullptr);
        m_allocation = std::exchange(other.m_allocation, {});
        m_allocation_info = std::exchange(other.m_allocation_info, {});
    }
    return *this;
}

VkStagingBuffer::~VkStagingBuffer()
{
    if (m_handle != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_device->allocator(), m_handle, m_allocation);
    }
}

auto VkStagingBuffer::handle() const -> ::VkBuffer
{
    return m_handle;
}

auto VkStagingBuffer::allocation() const -> VmaAllocation
{
    return m_allocation;
}

auto VkStagingBuffer::allocation_info() const -> VmaAllocationInfo const&
{
    return m_allocation_info;
}

}
