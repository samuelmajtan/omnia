/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Common/Expected.h>
#include <LibAsset/AssetManager.h>
#include <LibRHI/Device.h>
#include <LibRenderer/Export.h>
#include <LibRenderer/Model.h>

namespace Renderer {

class RENDERER_API ResourceManager final {
public:
    struct DefaultResource {
        static constexpr auto ALBEDO_TEXTURE_ID = Asset::AssetID(1);
        static constexpr auto NORMAL_TEXTURE_ID = Asset::AssetID(2);
        static constexpr auto METALLIC_ROUGHNESS_TEXTURE_ID = Asset::AssetID(3);
        static constexpr auto OCCLUSION_TEXTURE_ID = Asset::AssetID(4);
        static constexpr auto EMISSIVE_TEXTURE_ID = Asset::AssetID(5);
    };

    static auto create(Asset::AssetManager const* asset_manager, RHI::Device* device) -> Common::Expected<std::unique_ptr<ResourceManager>>;

    auto resource_layout() const -> RHI::ResourceLayout const*;
    auto load_model(std::string const& asset_name) -> Common::Expected<Model const*>;
    auto load_model(Asset::AssetID asset_id) -> Common::Expected<Model const*>;

    auto load_shader(std::string const& asset_name) -> Common::Expected<RHI::Shader const*>;
    auto load_shader(Asset::AssetID asset_id) -> Common::Expected<RHI::Shader const*>;

    auto load_texture(std::string const& asset_name, RHI::TextureFormat format = RHI::TextureFormat::R8G8B8A8_UNORM) -> Common::Expected<RHI::Texture const*>;
    auto load_texture(Asset::AssetID asset_id, RHI::TextureFormat format = RHI::TextureFormat::R8G8B8A8_UNORM) -> Common::Expected<RHI::Texture const*>;
private:
    ResourceManager() = default;
    auto initialize_default_resources() -> Common::Expected<void>;
private:
    RHI::Device* m_device {};
    Asset::AssetManager const* m_asset_manager {};
    std::unique_ptr<RHI::ResourceLayout> m_material_resource_layout;
    std::unordered_map<Asset::AssetID, std::unique_ptr<RHI::Texture>> m_texture_cache;
    std::unordered_map<Asset::AssetID, std::unique_ptr<RHI::Shader>> m_shader_cache;
    std::unordered_map<Asset::AssetID, std::unique_ptr<Model>> m_model_cache;
};

}
