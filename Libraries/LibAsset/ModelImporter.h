/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibAsset/AssetRegistry.h>
#include <LibAsset/Export.h>
#include <LibAsset/Importer.h>
#include <LibAsset/TextureImporter.h>
#include <LibGraphics/ModelTypes.h>

namespace Asset {

struct MaterialData {
    std::string name;
    std::optional<AssetID> albedo_texture_id = std::nullopt;
    std::optional<AssetID> metallic_roughness_texture_id = std::nullopt;
    std::optional<AssetID> normal_texture_id = std::nullopt;
    std::optional<AssetID> occlusion_texture_id = std::nullopt;
    std::optional<AssetID> emissive_texture_id = std::nullopt;
    Graphics::MaterialParameters parameters;
};

struct ModelData {
    std::vector<Graphics::SubMeshData> sub_meshes;
    std::vector<MaterialData> materials;
};

class ASSET_API ModelImporter final {
public:
    static constexpr u32 VERSION = 1;

public:
    static auto import(ImportContext const& context) -> std::expected<ModelData, std::string>;
    static auto source_hash(ImportContext const& context) -> std::expected<u64, std::string>;
    static auto supported_extensions() -> std::vector<std::string>;
private:
    static auto import_gltf(ImportContext const& context) -> std::expected<ModelData, std::string>;
};

}
