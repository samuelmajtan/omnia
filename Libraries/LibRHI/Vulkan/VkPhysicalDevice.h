/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <expected>
#include <vector>

#include <Common/Noncopyable.h>
#include <Common/Types.h>
#include <LibRHI/Vulkan/VkCommon.h>

namespace RHI {

class VkPhysicalDevice final {
    OA_MAKE_NONCOPYABLE(VkPhysicalDevice);
    OA_MAKE_DEFAULT_MOVABLE(VkPhysicalDevice);
    OA_MAKE_DEFAULT_CONSTRUCTIBLE(VkPhysicalDevice);

public:
    struct QueueFamilyIndices {
        u32 graphics = -1U;
        u32 present = -1U;
        u32 transfer = -1U;
    };

    VkPhysicalDevice(::VkPhysicalDevice handle, VkSurfaceKHR surface);
    ~VkPhysicalDevice() = default;

    auto is_suitable() const -> bool;
    auto handle() const -> ::VkPhysicalDevice;
    auto name() const -> std::string const&;
    auto queue_family_indices() const -> QueueFamilyIndices const&;
    auto surface_formats() const -> std::vector<VkSurfaceFormatKHR> const&;
    auto present_modes() const -> std::vector<VkPresentModeKHR> const&;
    auto surface_capabilities() const -> VkSurfaceCapabilitiesKHR;
private:
    ::VkPhysicalDevice m_handle {};
    std::string m_name {};
    VkSurfaceKHR m_surface {};
    QueueFamilyIndices m_queue_family_indices {};
    std::vector<VkSurfaceFormatKHR> m_surface_formats {};
    std::vector<VkPresentModeKHR> m_present_modes {};
};

auto to_string(VkPhysicalDeviceType type) -> std::string_view;

}
