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

class DX12Swapchain final : public Swapchain {
    OA_MAKE_NONCOPYABLE(DX12Swapchain);
    OA_MAKE_NONMOVABLE(DX12Swapchain);

public:
    static auto create(Configuration const& config) -> Common::Expected<std::unique_ptr<DX12Swapchain>>;

    ~DX12Swapchain() override;

    auto width() const -> u32 override;
    auto height() const -> u32 override;

    auto format() const -> TextureFormat override;
    auto textures() const -> std::vector<std::unique_ptr<Texture>> const& override;

    auto is_dirty() const -> bool override;
    auto recreate(Configuration const& config) -> Common::Expected<void> override;
    void wait_idle() const override;
    auto begin_frame() -> std::optional<Frame> override;
    void end_frame(Frame const& frame) override;
private:
    DX12Swapchain() = default;
private:
    Configuration m_config;
};

}
