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
#include <LibRHI/Shader.h>
#include <LibRHI/Vulkan/VkDevice.h>

namespace RHI {

class VkShader final : public Shader {
public:
    static auto create(Configuration const& config, RHI::VkDevice const* device) -> Common::Expected<std::unique_ptr<VkShader>>;

    ~VkShader() override;

    auto config() const -> Configuration const& override;
    auto handle() const -> VkShaderModule;
private:
    VkShader(Configuration const& config, RHI::VkDevice const* device);
private:
    Configuration m_config;
    RHI::VkDevice const* m_device {};
    VkShaderModule m_handle {};
};

auto to_vk(Shader const* shader) -> RHI::VkShader const*;
auto to_vk(Graphics::ShaderStageMask stages) -> VkShaderStageFlags;

}
