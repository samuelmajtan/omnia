/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Common/Expected.h>
#include <LibRHI/Metal/MTLBuffer.h>
#include <LibRHI/Metal/MTLDevice.h>
#include <LibRHI/Metal/MTLShader.h>
#include <LibRHI/Metal/MTLSwapchain.h>
#include <LibRHI/Metal/MTLTexture.h>

namespace RHI {

auto MTLDevice::create() -> Common::Expected<std::unique_ptr<MTLDevice>>
{
    std::unique_ptr<MTLDevice> device(new MTLDevice);
    return device;
}

MTLDevice::~MTLDevice()
{
}

auto MTLDevice::physical_devices() const -> std::vector<std::string_view>
{
    return {};
}

auto MTLDevice::select_physical_device(std::string_view name) -> bool
{
    (void)name;
    return false;
}

auto MTLDevice::create_buffer(Buffer::Configuration const& config) const -> Common::Expected<std::unique_ptr<Buffer>>
{
    return MTLBuffer::create(config);
}

auto MTLDevice::create_shader(Shader::Configuration const& config) const -> Common::Expected<std::unique_ptr<Shader>>
{
    return MTLShader::create(config);
}

auto MTLDevice::create_swapchain(Swapchain::Configuration const& config) const -> Common::Expected<std::unique_ptr<Swapchain>>
{
    return MTLSwapchain::create(config);
}

auto MTLDevice::create_texture(Texture::Configuration const& config) const -> Common::Expected<std::unique_ptr<Texture>>
{
    return MTLTexture::create(config);
}

}
