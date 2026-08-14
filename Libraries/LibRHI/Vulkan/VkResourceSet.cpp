/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <cassert>
#include <format>

#include <LibRHI/Vulkan/VkBuffer.h>
#include <LibRHI/Vulkan/VkResourceLayout.h>
#include <LibRHI/Vulkan/VkResourceSet.h>
#include <LibRHI/Vulkan/VkSampler.h>
#include <LibRHI/Vulkan/VkTexture.h>

namespace RHI {

auto VkResourceSet::create(Configuration const& config, RHI::VkDevice* device) -> std::expected<std::unique_ptr<VkResourceSet>, std::string>
{
    std::unique_ptr<VkResourceSet> resource_set(new VkResourceSet(config, device));

    auto* layout_handle = to_vk(config.layout)->handle();
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = device->descriptor_pool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &layout_handle,
    };

    auto result = vkAllocateDescriptorSets(device->handle(), &descriptor_set_allocate_info, &resource_set->m_handle);
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY) {
        if (auto new_pool = device->grow_descriptor_pool()) {
            descriptor_set_allocate_info.descriptorPool = *new_pool;
            if (auto regrow_result = vkAllocateDescriptorSets(device->handle(), &descriptor_set_allocate_info, &resource_set->m_handle); regrow_result != VK_SUCCESS) {
                return std::unexpected(std::format("Failed to allocate descriptor set after growing pool: {}", string_VkResult(result)));
            }
        }
    } else if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to allocate descriptor set: {}", string_VkResult(result)));
    }

    return resource_set;
}

VkResourceSet::VkResourceSet([[maybe_unused]] Configuration const& config, RHI::VkDevice const* device)
    : m_device(device)
{
    assert(device != nullptr);
    assert(config.layout != nullptr);
}

VkResourceSet::~VkResourceSet()
{
}

auto VkResourceSet::handle() const -> VkDescriptorSet
{
    return m_handle;
}

void VkResourceSet::set_sampler(u32 binding, const RHI::Sampler* sampler)
{
    VkDescriptorImageInfo const image_info {
        .sampler = to_vk(sampler)->handle(),
        .imageView = VK_NULL_HANDLE,
        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VkWriteDescriptorSet const write_descriptor_set {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_handle,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo = &image_info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr
    };

    vkUpdateDescriptorSets(m_device->handle(), 1, &write_descriptor_set, 0, nullptr);
}

void VkResourceSet::set_texture(u32 binding, Texture const* texture)
{
    VkDescriptorImageInfo const image_info {
        .sampler = nullptr,
        .imageView = to_vk(texture)->image_view(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet const write_descriptor_set {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_handle,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo = &image_info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr
    };

    vkUpdateDescriptorSets(m_device->handle(), 1, &write_descriptor_set, 0, nullptr);
}

void VkResourceSet::set_depth_texture(u32 binding, const RHI::Texture* texture)
{
    VkDescriptorImageInfo const image_info {
        .sampler = nullptr,
        .imageView = to_vk(texture)->image_view(),
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet const write_descriptor_set {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_handle,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo = &image_info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr
    };

    vkUpdateDescriptorSets(m_device->handle(), 1, &write_descriptor_set, 0, nullptr);
}

void VkResourceSet::set_uniform_buffer(u32 binding, Buffer const* buffer)
{
    VkDescriptorBufferInfo const buffer_info {
        .buffer = to_vk(buffer)->handle(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkWriteDescriptorSet const write_descriptor_set {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_handle,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = &buffer_info,
        .pTexelBufferView = nullptr
    };

    vkUpdateDescriptorSets(m_device->handle(), 1, &write_descriptor_set, 0, nullptr);
}

auto to_vk(ResourceSet const* resource_set) -> VkResourceSet const*
{
    return static_cast<VkResourceSet const*>(resource_set);
}

}
