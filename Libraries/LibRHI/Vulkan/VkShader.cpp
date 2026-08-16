/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <cassert>
#include <format>

#include <Common/Expected.h>
#include <LibRHI/Vulkan/VkShader.h>

namespace RHI {

auto VkShader::create(Configuration const& config, RHI::VkDevice const* device) -> Common::Expected<std::unique_ptr<VkShader>>
{
    std::unique_ptr<VkShader> shader(new VkShader(config, device));

    auto variant_it = std::ranges::find_if(config.variants.begin(), config.variants.end(), [](Graphics::ShaderVariant const& variant) {
        return variant.format == Graphics::ShaderFormat::SPIRV;
    });
    VkShaderModuleCreateInfo const shader_module_create_info {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = variant_it->bytecode.size(),
        .pCode = reinterpret_cast<u32 const*>(variant_it->bytecode.data())
    };
    if (auto result = vkCreateShaderModule(device->handle(), &shader_module_create_info, nullptr, &shader->m_handle); result != VK_SUCCESS) {
        return OA_ERROR("Failed to create vulkan shader module: {}", string_VkResult(result));
    }

    return shader;
}

VkShader::VkShader(Configuration const& config, RHI::VkDevice const* device)
    : m_config(config)
    , m_device(device)
{
    assert(device != nullptr);
    assert(!config.variants.empty());

    [[maybe_unused]] auto variant_it = std::ranges::find_if(config.variants.begin(), config.variants.end(), [](Graphics::ShaderVariant const& variant) {
        return variant.format == Graphics::ShaderFormat::SPIRV;
    });
    assert(variant_it != config.variants.end());
    assert(variant_it->bytecode.size() % 4 == 0);
}

VkShader::~VkShader()
{
    if (m_handle != nullptr) {
        vkDestroyShaderModule(m_device->handle(), m_handle, nullptr);
    }
}

auto VkShader::config() const -> Configuration const&
{
    return m_config;
}

auto VkShader::handle() const -> VkShaderModule
{
    return m_handle;
}

auto to_vk(Shader const* shader) -> RHI::VkShader const*
{
    return static_cast<RHI::VkShader const*>(shader);
}

auto to_vk(Graphics::ShaderStageMask stages) -> VkShaderStageFlags
{
    using enum Graphics::ShaderStageMask;

    VkShaderStageFlags flags {};
    if (any(stages & Vertex)) {
        flags |= VK_SHADER_STAGE_VERTEX_BIT;
    }
    if (any(stages & Fragment)) {
        flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    return flags;
}

}
