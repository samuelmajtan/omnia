/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Common/Expected.h>
#include <LibRHI/Metal/MTLShader.h>

namespace RHI {

auto MTLShader::create(Configuration const& config) -> Common::Expected<std::unique_ptr<MTLShader>>
{
    std::unique_ptr<MTLShader> shader(new MTLShader);
    shader->m_config = config;
    return shader;
}

MTLShader::~MTLShader()
{
}

auto MTLShader::config() const -> Configuration const&
{
    return m_config;
}

}
