/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <expected>
#include <filesystem>
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
};

class ASSET_API AssetRegistry final {
    OA_MAKE_DEFAULT_CONSTRUCTIBLE(AssetRegistry);

public:
    static constexpr char SUB_ASSET_SEPARATOR = '#';

    explicit AssetRegistry(std::filesystem::path const& root_directory);

    auto scan() -> Common::Expected<void>;

    auto register_asset(AssetEntry const& entry) -> Common::Expected<void>;
    auto register_sub_asset(AssetEntry const& parent, SubAssetDescriptor const& descriptor) -> Common::Expected<void>;
    auto key_to_id(std::string const& key) const -> std::optional<AssetID>;
    auto resolve(AssetID id) const -> Common::Expected<AssetEntry>;
    auto resolve_key(std::filesystem::path path) const -> std::string;
    static auto sub_asset_key(std::string const& parent_key, std::string const& sub_asset_name) -> std::string;

    auto entries() const -> std::unordered_map<std::string, AssetEntry> const&;
private:
    std::filesystem::path m_root_directory;
    std::unordered_map<std::string, AssetEntry> m_assets_by_key;
    std::unordered_map<AssetID, std::string> m_keys_by_id;
};

}
