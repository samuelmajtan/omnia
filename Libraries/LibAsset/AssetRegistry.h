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

#include <Common/Noncopyable.h>
#include <Common/Types.h>
#include <LibAsset/Asset.h>
#include <LibAsset/AssetSidecar.h>
#include <LibAsset/Export.h>

namespace Asset {

struct LooseAssetEntry {
    std::filesystem::path path;
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
};

class ASSET_API AssetRegistry final {
    OA_MAKE_DEFAULT_CONSTRUCTIBLE(AssetRegistry);

public:
    explicit AssetRegistry(std::filesystem::path const& root_directory);

    auto scan() -> std::expected<void, std::string>;

    auto register_asset(AssetEntry const& entry) -> std::expected<void, std::string>;
    auto key_to_id(std::string const& key) const -> std::optional<AssetID>;
    auto resolve(AssetID id) const -> std::expected<AssetEntry, std::string>;
    auto resolve_key(std::filesystem::path path) const -> std::string;

    auto entries() const -> std::unordered_map<std::string, AssetEntry> const&;
private:
    std::filesystem::path m_root_directory;
    std::unordered_map<std::string, AssetEntry> m_assets_by_key;
    std::unordered_map<AssetID, std::string> m_keys_by_id;
};

}
