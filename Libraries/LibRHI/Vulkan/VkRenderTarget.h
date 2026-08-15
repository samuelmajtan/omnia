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
#include <LibRHI/RenderTarget.h>
#include <LibRHI/Vulkan/VkDevice.h>

namespace RHI {

class VkRenderTarget final : public RenderTarget {
public:
    static auto create(Configuration const& config, RHI::VkDevice const* device) -> Common::Expected<std::unique_ptr<VkRenderTarget>>;
    ~VkRenderTarget() override;

    auto framebuffer() const -> VkFramebuffer;
    auto width() const -> u32 override;
    auto height() const -> u32 override;
private:
    VkRenderTarget(Configuration const& config, RHI::VkDevice const* device);
private:
    RHI::VkDevice const* m_device {};
    VkFramebuffer m_framebuffer {};
    u32 m_width {};
    u32 m_height {};
};

auto to_vk(RenderTarget const* render_target) -> VkRenderTarget const*;

}
