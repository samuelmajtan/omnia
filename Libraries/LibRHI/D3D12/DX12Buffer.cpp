/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Common/Expected.h>
#include <LibRHI/D3D12/DX12Buffer.h>

namespace RHI {

auto DX12Buffer::create(Configuration const& config) -> Common::Expected<std::unique_ptr<DX12Buffer>>
{
    std::unique_ptr<DX12Buffer> buffer(new DX12Buffer);
    buffer->m_config = config;
    return buffer;
}

DX12Buffer::~DX12Buffer()
{
}

void DX12Buffer::set_data(void const* data, u64 size)
{
    m_config.data = data;
    m_config.size = size;
}

}
