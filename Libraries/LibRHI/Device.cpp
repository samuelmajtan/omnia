/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Common/Expected.h>
#include <Common/Platform.h>
#include <LibRHI/Device.h>
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
        return DX12Device::create();
#elifdef OA_OS_MACOS
    case API::Metal:
        return MTLDevice::create();
#endif
#if defined(OA_OS_LINUX) || defined(OA_OS_WINDOWS)
    case API::Vulkan:
        return VkDevice::create(config);
#endif
    default:
        return OA_ERROR("Unsupported graphics API selected");
    }
}

}
