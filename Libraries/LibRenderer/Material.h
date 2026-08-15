/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Common/Expected.h>
#include <LibGraphics/ModelTypes.h>
#include <LibRHI/Device.h>
#include <LibRenderer/Export.h>

namespace Renderer {

class RENDERER_API Material final {
    OA_MAKE_NONCOPYABLE(Material);
    OA_MAKE_DEFAULT_MOVABLE(Material);
    OA_MAKE_DEFAULT_DESTRUCTIBLE(Material);

public:
    struct Configuration {
        std::string name;
        RHI::Texture const* albedo_texture = nullptr;
        RHI::Texture const* metallic_roughness_texture = nullptr;
        RHI::Texture const* normal_texture = nullptr;
        RHI::Texture const* occlusion_texture = nullptr;
        RHI::Texture const* emissive_texture = nullptr;
        RHI::ResourceLayout const* resource_layout = nullptr;
        Graphics::MaterialParameters parameters {};
    };

    static auto create(Configuration const& configuration, RHI::Device* device) -> Common::Expected<Material>;

    auto resource_set() const -> RHI::ResourceSet const*;
private:
    Material() = default;
private:
    Graphics::MaterialParameters m_parameters {};
    std::unique_ptr<RHI::Buffer> m_uniform_buffer;
    std::unique_ptr<RHI::ResourceSet> m_resource_set;
};

}
