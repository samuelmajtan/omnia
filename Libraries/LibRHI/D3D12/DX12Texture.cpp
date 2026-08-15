/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Common/Expected.h>
#include <LibRHI/D3D12/DX12Texture.h>

namespace RHI {

auto DX12Texture::create(Configuration const& config) -> Common::Expected<std::unique_ptr<DX12Texture>>
{
    std::unique_ptr<DX12Texture> texture(new DX12Texture);
    (void)config;
    return texture;
}

DX12Texture::~DX12Texture()
{
}

auto DX12Texture::width() const -> u32
{
    return 0;
}

auto DX12Texture::height() const -> u32
{
    return 0;
}

auto DX12Texture::format() const -> TextureFormat
{
    return TextureFormat::Unknown;
}

}
