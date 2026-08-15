/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Common/Expected.h>
#include <Common/Noncopyable.h>
#include <LibRHI/Device.h>

namespace RHI {

class MTLDevice final : public Device {
    OA_MAKE_NONCOPYABLE(MTLDevice);
    OA_MAKE_NONMOVABLE(MTLDevice);

public:
    static auto create() -> Common::Expected<std::unique_ptr<MTLDevice>>;

    ~MTLDevice() override;

    auto physical_devices() const -> std::vector<std::string_view> override;
    auto select_physical_device(std::string_view name) -> bool override;

    auto create_buffer(Buffer::Configuration const& config) const -> Common::Expected<std::unique_ptr<Buffer>> override;
    auto create_shader(Shader::Configuration const& config) const -> Common::Expected<std::unique_ptr<Shader>> override;
    auto create_swapchain(Swapchain::Configuration const& config) const -> Common::Expected<std::unique_ptr<Swapchain>> override;
    auto create_texture(Texture::Configuration const& config) const -> Common::Expected<std::unique_ptr<Texture>> override;
private:
    MTLDevice() = default;
};

}
