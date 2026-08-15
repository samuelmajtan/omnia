/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Common/Expected.h>
#include <LibRHI/Metal/MTLSwapchain.h>

namespace RHI {

auto MTLSwapchain::create(Configuration const& config) -> Common::Expected<std::unique_ptr<MTLSwapchain>>
{
    (void)config;
    std::unique_ptr<MTLSwapchain> swapchain(new MTLSwapchain);
    return swapchain;
}

MTLSwapchain::~MTLSwapchain()
{
}

void MTLSwapchain::present()
{
}

auto MTLSwapchain::config() const -> Configuration const&
{
    return m_config;
}

}
