/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Common/Expected.h>
#include <LibRHI/D3D12/DX12Shader.h>

namespace RHI {

auto DX12Shader::create(Configuration const& config) -> Common::Expected<std::unique_ptr<DX12Shader>>
{
    std::unique_ptr<DX12Shader> shader(new DX12Shader);
    shader->m_config = config;
    return shader;
}

DX12Shader::~DX12Shader()
{
}

auto DX12Shader::config() const -> Configuration const&
{
    return m_config;
}

}
