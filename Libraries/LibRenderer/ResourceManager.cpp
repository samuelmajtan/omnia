/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Common/Expected.h>
#include <LibDebug/Logger.h>
#include <LibRenderer/ResourceManager.h>

namespace Renderer {

namespace {

constexpr Debug::Logger Logger("Renderer");

}

auto ResourceManager::create(Asset::AssetManager const* asset_manager, RHI::Device* device) -> Common::Expected<std::unique_ptr<ResourceManager>>
{
    assert(asset_manager != nullptr);
    assert(device != nullptr);

    std::unique_ptr<ResourceManager> resource_manager(new ResourceManager);
    resource_manager->m_asset_manager = asset_manager;
    resource_manager->m_device = device;

    RHI::ResourceLayout::Configuration material_resource_layout_config {
        .bindings = {
            {
                .binding = 0,
                .type = RHI::ResourceType::UniformBuffer,
                .stage = Graphics::ShaderStage::Fragment
            },
        }
    };

    u32 const texture_count = 5U;
    for (u32 i = 1; i <= texture_count; i++) {
        material_resource_layout_config.bindings.push_back(
            {
                .binding = i,
                .type = RHI::ResourceType::Texture,
                .stage = Graphics::ShaderStage::Fragment
            }
        );
    }

    resource_manager->m_material_resource_layout = TRY(device->create_resource_layout(material_resource_layout_config));
    TRY(resource_manager->initialize_default_resources());
    return resource_manager;
}

auto ResourceManager::resource_layout() const -> RHI::ResourceLayout const*
{
    return m_material_resource_layout.get();
}

auto ResourceManager::load_model(std::string const& asset_name) -> Common::Expected<Model const*>
{
    auto asset_id_result = m_asset_manager->registry().key_to_id(asset_name);
    if (!asset_id_result.has_value()) {
        return OA_ERROR("Failed to find asset with name '{}'", asset_name);
    }
    return load_model(asset_id_result.value());
}

auto ResourceManager::load_model(Asset::AssetID asset_id) -> Common::Expected<Model const*>
{
    if (auto const it = m_model_cache.find(asset_id); it != m_model_cache.end()) {
        return it->second.get();
    }

    auto const model_data = TRY(m_asset_manager->import<Asset::ModelData>(asset_id));

    auto resolve_texture = [&](std::optional<Asset::AssetID> const& texture_id, Asset::AssetID default_id) -> RHI::Texture const* {
        assert(m_texture_cache.contains(default_id));

        if (!texture_id.has_value()) {
            return m_texture_cache[default_id].get();
        }
        RHI::TextureFormat format = RHI::TextureFormat::R8G8B8A8_UNORM;
        if (default_id == DefaultResource::ALBEDO_TEXTURE_ID) {
            format = RHI::TextureFormat::R8G8B8A8_SRGB;
        }
        auto const texture_result = load_texture(texture_id.value(), format);
        if (!texture_result) {
            Logger.warn("Falling back to a default texture: {} for model id: {}", texture_result.error(), asset_id);
            return m_texture_cache[default_id].get();
        }
        return texture_result.value();
    };

    Model::Configuration model_config {
        .sub_meshes = model_data.sub_meshes,
        .materials = {}
    };
    model_config.materials.reserve(model_data.materials.size());
    for (auto const& material_data : model_data.materials) {
        Material::Configuration material_config {
            .name = material_data.name,
            .albedo_texture = resolve_texture(material_data.albedo_texture_id, DefaultResource::ALBEDO_TEXTURE_ID),
            .metallic_roughness_texture = resolve_texture(material_data.metallic_roughness_texture_id, DefaultResource::METALLIC_ROUGHNESS_TEXTURE_ID),
            .normal_texture = resolve_texture(material_data.normal_texture_id, DefaultResource::NORMAL_TEXTURE_ID),
            .occlusion_texture = resolve_texture(material_data.occlusion_texture_id, DefaultResource::OCCLUSION_TEXTURE_ID),
            .emissive_texture = resolve_texture(material_data.emissive_texture_id, DefaultResource::EMISSIVE_TEXTURE_ID),
            .resource_layout = m_material_resource_layout.get(),
            .parameters = material_data.parameters
        };
        model_config.materials.push_back(std::move(material_config));
    }

    m_model_cache[asset_id] = TRY(Model::create(model_config, m_device));
    return m_model_cache[asset_id].get();
}

auto ResourceManager::load_shader(std::string const& asset_name) -> Common::Expected<RHI::Shader const*>
{
    auto asset_id_result = m_asset_manager->registry().key_to_id(asset_name);
    if (!asset_id_result.has_value()) {
        return OA_ERROR("Failed to find asset with name '{}'", asset_name);
    }
    return load_shader(asset_id_result.value());
}

auto ResourceManager::load_shader(Asset::AssetID asset_id) -> Common::Expected<RHI::Shader const*>
{
    if (auto const it = m_shader_cache.find(asset_id); it != m_shader_cache.end()) {
        return it->second.get();
    }

    auto const shader_data = TRY(m_asset_manager->import<RHI::Shader::Configuration>(asset_id));
    m_shader_cache[asset_id] = TRY(m_device->create_shader(shader_data));
    return m_shader_cache[asset_id].get();
}

auto ResourceManager::load_texture(std::string const& asset_name, RHI::TextureFormat format) -> Common::Expected<const RHI::Texture*>
{
    auto asset_id_result = m_asset_manager->registry().key_to_id(asset_name);
    if (!asset_id_result.has_value()) {
        return OA_ERROR("Failed to find asset with name '{}'", asset_name);
    }
    return load_texture(asset_id_result.value(), format);
}

auto ResourceManager::load_texture(Asset::AssetID asset_id, RHI::TextureFormat format) -> Common::Expected<RHI::Texture const*>
{
    if (auto const it = m_texture_cache.find(asset_id); it != m_texture_cache.end()) {
        return it->second.get();
    }

    auto const texture_data = TRY(m_asset_manager->import<Asset::TextureData>(asset_id));
    RHI::Texture::Configuration const texture_config {
        .width = texture_data.width,
        .height = texture_data.height,
        .format = format,
        .usage = RHI::TextureUsage::Sampled,
        .data = texture_data.data
    };

    m_texture_cache[asset_id] = TRY(m_device->create_texture(texture_config));
    return m_texture_cache[asset_id].get();
}

auto ResourceManager::initialize_default_resources() -> Common::Expected<void>
{
    RHI::Texture::Configuration const default_texture_config {
        .width = 1,
        .height = 1,
        .format = RHI::TextureFormat::R8G8B8A8_SRGB,
        .usage = RHI::TextureUsage::Sampled,
        .data = { 255, 255, 255, 255 }
    };
    RHI::Texture::Configuration const default_normal_texture_config {
        .width = 1,
        .height = 1,
        .format = RHI::TextureFormat::R8G8B8A8_UNORM,
        .usage = RHI::TextureUsage::Sampled,
        .data = { 128, 128, 255, 255 }
    };
    RHI::Texture::Configuration const default_metallic_roughness_texture_config {
        .width = 1,
        .height = 1,
        .format = RHI::TextureFormat::R8G8B8A8_UNORM,
        .usage = RHI::TextureUsage::Sampled,
        .data = { 255, 255, 0, 255 }
    };
    RHI::Texture::Configuration const default_occlusion_texture_config {
        .width = 1,
        .height = 1,
        .format = RHI::TextureFormat::R8G8B8A8_UNORM,
        .usage = RHI::TextureUsage::Sampled,
        .data = { 255, 255, 255, 255 }
    };
    RHI::Texture::Configuration const default_emissive_texture_config {
        .width = 1,
        .height = 1,
        .format = RHI::TextureFormat::R8G8B8A8_UNORM,
        .usage = RHI::TextureUsage::Sampled,
        .data = { 0, 0, 0, 255 }
    };

    std::unordered_map<Asset::AssetID, RHI::Texture::Configuration> const default_textures = {
        { DefaultResource::ALBEDO_TEXTURE_ID, default_texture_config },
        { DefaultResource::NORMAL_TEXTURE_ID, default_normal_texture_config },
        { DefaultResource::METALLIC_ROUGHNESS_TEXTURE_ID, default_metallic_roughness_texture_config },
        { DefaultResource::OCCLUSION_TEXTURE_ID, default_occlusion_texture_config },
        { DefaultResource::EMISSIVE_TEXTURE_ID, default_emissive_texture_config }
    };

    for (auto const& [asset_id, texture_config] : default_textures) {
        if (auto const it = m_texture_cache.find(asset_id); it != m_texture_cache.end()) {
            continue;
        }

        m_texture_cache[asset_id] = TRY(m_device->create_texture(texture_config));
    }
    return {};
}

}
