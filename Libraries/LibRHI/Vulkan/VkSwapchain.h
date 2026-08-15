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
#include <LibRHI/Swapchain.h>
#include <LibRHI/Vulkan/VkCommandBuffer.h>
#include <LibRHI/Vulkan/VkDevice.h>
#include <LibRHI/Vulkan/VkTexture.h>

namespace RHI {

class VkSwapchain final : public Swapchain {
public:
    static auto create(Configuration const& config, RHI::VkDevice const* device) -> Common::Expected<std::unique_ptr<VkSwapchain>>;

    ~VkSwapchain() override;

    auto width() const -> u32 override;
    auto height() const -> u32 override;

    auto format() const -> TextureFormat override;
    auto textures() const -> std::vector<std::unique_ptr<Texture>> const& override;

    auto is_dirty() const -> bool override;
    auto recreate(Configuration const& config) -> Common::Expected<void> override;
    void wait_idle() const override;
    auto begin_frame() -> std::optional<Frame> override;
    void end_frame(Frame const& frame) override;
private:
    VkSwapchain(Configuration const& config, RHI::VkDevice const* device);

    auto select_surface_format() const -> VkSurfaceFormatKHR;
    auto select_present_mode() const -> VkPresentModeKHR;
    auto select_swap_extent() const -> VkExtent2D;
    auto select_image_count() const -> u32;

    auto create_swapchain() -> Common::Expected<void>;
    auto create_images() -> Common::Expected<void>;
    auto create_command_buffers() -> Common::Expected<void>;
    auto create_sync_objects() -> Common::Expected<void>;
private:
    Configuration m_config;
    bool m_is_dirty = false;
    VkSurfaceFormatKHR m_surface_format {};
    VkExtent2D m_extent {};
    VkSwapchainKHR m_handle {};
    RHI::VkDevice const* m_device {};
    std::vector<VkImage> m_images;
    std::vector<std::unique_ptr<Texture>> m_textures;

    std::vector<RHI::VkCommandBuffer> m_command_buffers;
    std::vector<VkSemaphore> m_image_available_semaphores;
    std::vector<VkSemaphore> m_render_finished_semaphores;
    std::vector<VkFence> m_in_flight_fences;
    u32 m_current_frame = 0;
};

}
