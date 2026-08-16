/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <expected>
#include <filesystem>
#include <span>
#include <unordered_map>
#include <variant>
#include <vector>

#include <Common/Expected.h>
#include <Common/Noncopyable.h>
#include <Common/Types.h>
#include <LibAsset/Asset.h>
#include <LibAsset/AssetSidecar.h>
#include <LibAsset/Export.h>
#include <LibAsset/Importer.h>

namespace Asset {

struct LooseAssetEntry {
    std::filesystem::path path;
    std::optional<std::string> sub_asset = std::nullopt;
};

struct PackedAssetEntry {
    u64 offset;
    u64 size;
};

struct AssetEntry {
    AssetSidecar sidecar;
    std::string key;
    std::variant<LooseAssetEntry, PackedAssetEntry> source;

    auto id() const -> AssetID
    {
        return sidecar.id();
    }

    auto type() const -> AssetType
    {
        return sidecar.type();
    }

    auto sub_asset_name() const -> std::optional<std::string>
    {
        auto const* loose = std::get_if<LooseAssetEntry>(&source);
        return loose != nullptr ? loose->sub_asset : std::nullopt;
    }

    auto display_name() const -> std::string
    {
        auto const* loose = std::get_if<LooseAssetEntry>(&source);
        if (loose == nullptr) {
            return key;
        }

        auto name = sidecar.setting(AssetSidecar::NAME_SETTING).value_or(loose->path.stem().string());
        if (loose->sub_asset.has_value()) {
            return std::format("{}{}{}", name, SUB_ASSET_SEPARATOR, loose->sub_asset.value());
        }
        return name;
    }
};

class ASSET_API AssetRegistry final {
    OA_MAKE_DEFAULT_CONSTRUCTIBLE(AssetRegistry);

public:
    explicit AssetRegistry(std::filesystem::path const& root_directory);

    auto scan() -> Common::Expected<void>;

    auto register_asset(AssetEntry const& entry) -> Common::Expected<void>;
    auto register_sub_asset(AssetEntry const& parent, SubAssetDescriptor const& descriptor) -> Common::Expected<void>;
    auto key_to_id(std::string const& key) const -> std::optional<AssetID>;
    auto sub_assets_of(AssetID parent, AssetType sub_asset_type) const -> std::vector<AssetID>;
    auto resolve(AssetID id) const -> Common::Expected<AssetEntry>;
    auto resolve_sub_asset_key(std::string const& parent_key, std::string const& sub_asset_name) -> std::string;
    auto resolve_key(std::filesystem::path path) const -> std::string;
    auto entries() const -> std::unordered_map<AssetID, AssetEntry> const&;
private:
    std::filesystem::path m_root_directory;
    std::unordered_map<AssetID, AssetEntry> m_assets_by_id;
    std::unordered_map<std::string, AssetID> m_ids_by_key;
    std::unordered_map<AssetID, std::vector<AssetID>> m_sub_assets_by_parent;
};

}
