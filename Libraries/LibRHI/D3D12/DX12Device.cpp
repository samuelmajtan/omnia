/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Common/Expected.h>
#include <LibRHI/D3D12/DX12Buffer.h>
#include <LibRHI/D3D12/DX12Device.h>
#include <LibRHI/D3D12/DX12Shader.h>
#include <LibRHI/D3D12/DX12Swapchain.h>
#include <LibRHI/D3D12/DX12Texture.h>

namespace RHI {

auto DX12Device::create() -> Common::Expected<std::unique_ptr<DX12Device>>
{
    std::unique_ptr<DX12Device> device(new DX12Device);
    return device;
}

DX12Device::~DX12Device()
{
}

auto DX12Device::physical_devices() const -> std::vector<std::string_view>
{
    return {};
}

auto DX12Device::select_physical_device(std::string_view name) -> bool
{
    (void)name;
    return false;
}

auto DX12Device::create_buffer(Buffer::Configuration const& config) const -> Common::Expected<std::unique_ptr<Buffer>>
{
    return DX12Buffer::create(config);
}

auto DX12Device::create_pipeline(Pipeline::Configuration const& config) const -> Common::Expected<std::unique_ptr<Pipeline>>
{
    (void)config;
    return {};
}

auto DX12Device::create_render_pass(RenderPass::Configuration const& config) const -> Common::Expected<std::unique_ptr<RenderPass>>
{
    (void)config;
    return {};
}

auto DX12Device::create_render_target(RenderTarget::Configuration const& config) const -> Common::Expected<std::unique_ptr<RenderTarget>>
{
    (void)config;
    return {};
}

auto DX12Device::create_resource_layout(ResourceLayout::Configuration const& config) const -> Common::Expected<std::unique_ptr<ResourceLayout>>
{
    (void)config;
    return {};
}

auto DX12Device::create_resource_set(ResourceSet::Configuration const& config) -> Common::Expected<std::unique_ptr<ResourceSet>>
{
    (void)config;
    return {};
}

auto DX12Device::create_sampler(Sampler::Configuration const& config) const -> Common::Expected<std::unique_ptr<Sampler>>
{
    (void)config;
    return {};
}

auto DX12Device::create_shader(Shader::Configuration const& config) const -> Common::Expected<std::unique_ptr<Shader>>
{
    return DX12Shader::create(config);
}

auto DX12Device::create_swapchain(Swapchain::Configuration const& config) const -> Common::Expected<std::unique_ptr<Swapchain>>
{
    return DX12Swapchain::create(config);
}

auto DX12Device::create_texture(Texture::Configuration const& config) const -> Common::Expected<std::unique_ptr<Texture>>
{
    return DX12Texture::create(config);
}

}
