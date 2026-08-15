/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <expected>
#include <memory>
#include <string>

#include <Common/Expected.h>
#include <Common/Noncopyable.h>
#include <LibRHI/Swapchain.h>

namespace RHI {

class MTLSwapchain final : public Swapchain {
    OA_MAKE_NONCOPYABLE(MTLSwapchain);
    OA_MAKE_NONMOVABLE(MTLSwapchain);

public:
    static auto create(Configuration const& config) -> Common::Expected<std::unique_ptr<MTLSwapchain>>;

    ~MTLSwapchain() override;

    void present() override;
    auto config() const -> Configuration const& override;
private:
    MTLSwapchain() = default;
private:
    Configuration m_config;
};

}
