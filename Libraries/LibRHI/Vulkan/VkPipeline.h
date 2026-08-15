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
#include <LibRHI/Pipeline.h>
#include <LibRHI/Vulkan/VkCommon.h>

namespace RHI {

class VkDevice;

class VkPipeline final : public Pipeline {
public:
    static auto create(Configuration const& config, RHI::VkDevice const* device) -> Common::Expected<std::unique_ptr<VkPipeline>>;

    ~VkPipeline() override;

    auto handle() const -> ::VkPipeline;
    auto layout() const -> VkPipelineLayout;
private:
    VkPipeline(Configuration const& config, RHI::VkDevice const* device);
private:
    Configuration m_config;
    RHI::VkDevice const* m_device {};
    ::VkPipeline m_handle {};
    VkPipelineLayout m_layout {};
};

auto to_vk(Pipeline const* pipeline) -> VkPipeline const*;
auto to_vk(CullMode cull_mode) -> VkCullModeFlags;
auto to_vk(FrontFace front_face) -> VkFrontFace;
auto to_vk(PolygonMode polygon_mode) -> VkPolygonMode;
auto to_vk(CompareOp compare_op) -> VkCompareOp;
auto to_vk(AttributeFormat format) -> VkFormat;

}
