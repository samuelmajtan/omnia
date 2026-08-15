/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <unordered_map>
#include <vk_mem_alloc.h>

#include <Common/Expected.h>
#include <Common/Noncopyable.h>
#include <LibRHI/Device.h>
#include <LibRHI/Vulkan/VkCommandBuffer.h>
#include <LibRHI/Vulkan/VkCommon.h>
#include <LibRHI/Vulkan/VkPhysicalDevice.h>

namespace RHI {

class VkDevice final : public Device {
public:
    static auto create(Configuration const& config) -> Common::Expected<std::unique_ptr<VkDevice>>;

    ~VkDevice() override;

    auto descriptor_pool() const -> VkDescriptorPool;
    auto grow_descriptor_pool() -> Common::Expected<VkDescriptorPool>;
    auto allocator() const -> VmaAllocator;
    auto handle() const -> ::VkDevice;
    auto surface() const -> VkSurfaceKHR;

    void submit_graphics(RHI::VkCommandBuffer const& command_buffer) const;
    auto graphics_command_buffer() const -> RHI::VkCommandBuffer const&;

    auto graphics_queue() const -> VkQueue;
    auto present_queue() const -> VkQueue;
    auto transfer_queue() const -> VkQueue;

    auto graphics_pool() const -> VkCommandPool;
    auto transfer_pool() const -> VkCommandPool;

    auto selected_physical_device() const -> VkPhysicalDevice const*;
    auto physical_devices() const -> std::vector<std::string_view> override;
    auto select_physical_device(std::string_view name) -> bool override;

    auto create_buffer(Buffer::Configuration const& config) const -> Common::Expected<std::unique_ptr<Buffer>> override;
    auto create_pipeline(Pipeline::Configuration const& config) const -> Common::Expected<std::unique_ptr<Pipeline>> override;
    auto create_render_pass(RenderPass::Configuration const& config) const -> Common::Expected<std::unique_ptr<RenderPass>> override;
    auto create_render_target(RenderTarget::Configuration const& config) const -> Common::Expected<std::unique_ptr<RenderTarget>> override;
    auto create_resource_layout(ResourceLayout::Configuration const& config) const -> Common::Expected<std::unique_ptr<ResourceLayout>> override;
    auto create_resource_set(ResourceSet::Configuration const& config) -> Common::Expected<std::unique_ptr<ResourceSet>> override;
    auto create_sampler(Sampler::Configuration const& config) const -> Common::Expected<std::unique_ptr<Sampler>> override;
    auto create_shader(Shader::Configuration const& config) const -> Common::Expected<std::unique_ptr<Shader>> override;
    auto create_swapchain(Swapchain::Configuration const& config) const -> Common::Expected<std::unique_ptr<Swapchain>> override;
    auto create_texture(Texture::Configuration const& config) const -> Common::Expected<std::unique_ptr<Texture>> override;
private:
    VkDevice(Configuration const& config);

    auto create_instance() -> Common::Expected<void>;
    auto create_surface() -> Common::Expected<void>;
    auto create_logical_device() -> Common::Expected<void>;
    auto create_allocator() -> Common::Expected<void>;
    auto create_command_pools() -> Common::Expected<void>;
    auto create_descriptor_pool() -> Common::Expected<void>;
private:
    Configuration m_config {};
    VmaAllocator m_allocator {};
    VkInstance m_instance {};
    VkSurfaceKHR m_surface {};
    std::vector<VkDescriptorPool> m_descriptor_pools;
    u32 m_descriptor_pool_capacity = 16U;
    VkDebugUtilsMessengerEXT m_debug_messenger {};

    RHI::VkPhysicalDevice* m_physical_device {};
    ::VkDevice m_logical_device {};
    std::unordered_map<std::string_view, RHI::VkPhysicalDevice> m_physical_devices;

    VkQueue m_graphics_queue {};
    VkQueue m_present_queue {};
    VkQueue m_transfer_queue {};
    VkCommandPool m_graphics_command_pool {};
    VkCommandPool m_transfer_command_pool {};
    RHI::VkCommandBuffer m_graphics_command_buffer {};
};

}
