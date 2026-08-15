/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Common/Expected.h>
#include <LibRHI/Metal/MTLBuffer.h>

namespace RHI {

auto MTLBuffer::create(Configuration const& config) -> Common::Expected<std::unique_ptr<MTLBuffer>>
{
    std::unique_ptr<MTLBuffer> buffer(new MTLBuffer);
    buffer->m_config = config;
    return buffer;
}

MTLBuffer::~MTLBuffer()
{
}

void MTLBuffer::map()
{
}

void MTLBuffer::unmap()
{
}

auto MTLBuffer::config() const -> Configuration const&
{
    return m_config;
}

}
