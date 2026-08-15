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
#include <LibRHI/RenderPass.h>
#include <LibRHI/Vulkan/VkCommon.h>
#include <LibRHI/Vulkan/VkDevice.h>

namespace RHI {

class VkRenderPass final : public RenderPass {
public:
    static auto create(Configuration const& config, RHI::VkDevice const* device) -> Common::Expected<std::unique_ptr<RHI::VkRenderPass>>;

    ~VkRenderPass() override;

    auto handle() const -> ::VkRenderPass;

    void begin(CommandBuffer const* command_buffer, RenderTarget const* render_target) const override;
    void end(CommandBuffer const* command_buffer) const override;
private:
    VkRenderPass(Configuration const& config, RHI::VkDevice const* device);

    auto create_render_pass() -> Common::Expected<void>;
private:
    Configuration m_config {};
    ::VkRenderPass m_handle {};
    RHI::VkDevice const* m_device {};
    std::vector<VkClearValue> m_clear_values;
};

auto to_vk(LoadOp load_op) -> VkAttachmentLoadOp;
auto to_vk(StoreOp store_op) -> VkAttachmentStoreOp;
auto to_vk(ImageLayout layout) -> VkImageLayout;
auto to_vk(RenderPass const* render_pass) -> RHI::VkRenderPass const*;

}
