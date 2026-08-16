/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Common/Expected.h>
#include <Common/Platform.h>
#include <LibRHI/Device.h>
#include <LibRHI/Log.h>
#ifdef OA_OS_WINDOWS
#    include <LibRHI/D3D12/DX12Device.h>
#elifdef OA_OS_MACOS
#    include <LibRHI/Metal/MTLDevice.h>
#endif
#if defined(OA_OS_LINUX) || defined(OA_OS_WINDOWS)
#    include <LibRHI/Vulkan/VkDevice.h>
#endif

namespace RHI {

auto Device::create(Configuration const& config) -> Common::Expected<std::unique_ptr<Device>>
{
    switch (config.api) {
#ifdef OA_OS_WINDOWS
    case API::D3D12:
        OA_LOG_WARN(Log::Device, "The D3D12 backend is not yet supported");
        return DX12Device::create();
#elifdef OA_OS_MACOS
    case API::Metal:
        OA_LOG_WARN(Log::Device, "The Metal backend is not yet supported");
        return MTLDevice::create();
#endif
#if defined(OA_OS_LINUX) || defined(OA_OS_WINDOWS)
    case API::Vulkan:
        OA_LOG_INFO(Log::Device, "Graphics API: Vulkan (debug layer {})", config.enable_debug_layer ? "enabled" : "disabled");
        return VkDevice::create(config);
#endif
    default:
        return OA_ERROR("Unsupported graphics API selected");
    }
}

}
