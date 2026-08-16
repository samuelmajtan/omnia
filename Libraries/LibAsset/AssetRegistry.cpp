/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <algorithm>
#include <format>

#include <Common/Expected.h>
#include <Common/Time.h>
#include <LibAsset/AssetRegistry.h>
#include <LibAsset/AssetTraits.h>
#include <LibAsset/Log.h>

namespace Asset {

AssetRegistry::AssetRegistry(std::filesystem::path const& root_directory)
    : m_root_directory(root_directory)
{
}

auto AssetRegistry::scan() -> Common::Expected<void>
{
    if (m_root_directory.empty()) {
        OA_LOG_WARN(Log::Registry, "No root directory is configured");
        return {};
    }

    std::error_code error;
    auto iterator = std::filesystem::recursive_directory_iterator(m_root_directory, error);
    if (error) {
        return OA_ERROR("Failed to scan {}: {}", m_root_directory.string(), error.message());
    }

    Time::Stopwatch const stopwatch;
    for (auto const& entry : iterator) {
        if (!entry.is_regular_file()) {
            continue;
        }

        if (entry.path().extension() == AssetSidecar::SIDECAR_EXTENSION) {
            OA_LOG_TRACE(Log::Registry, "Skipping {}: sidecar file", entry.path().string());
            continue;
        }

        auto const type = asset_type_for(entry.path());
        if (!type) {
            OA_LOG_TRACE(Log::Registry, "Skipping {}: no importer claims this extension", entry.path().string());
            continue;
        }

        auto const sidecar_path = AssetSidecar::path_for(entry.path());
        AssetSidecar sidecar;

        if (std::filesystem::exists(sidecar_path)) {
            sidecar = TRY(AssetSidecar::load(sidecar_path));
        } else {
            sidecar = AssetSidecar(Common::UUID::generate(), type.value());
            TRY(sidecar.save(sidecar_path));
            OA_LOG_DEBUG(Log::Registry, "Assigned {} for {}", sidecar.id(), sidecar_path.string());
        }

        AssetEntry const asset_entry {
            .sidecar = std::move(sidecar),
            .key = resolve_key(entry.path()),
            .source = LooseAssetEntry { .path = entry.path() }
        };

        OA_LOG_TRACE(Log::Registry, "{} -> {} ({})", asset_entry.key, asset_entry.id(), to_string(asset_entry.type()));
        if (auto result = register_asset(asset_entry); !result.has_value()) {
            OA_LOG_WARN(Log::Registry, "{}", result.error());
            continue;
        }
    }

    return {};
}

auto AssetRegistry::register_asset(AssetEntry const& entry) -> Common::Expected<void>
{
    if (auto const existing = m_assets_by_key.find(entry.key); existing != m_assets_by_key.end()) {
        return OA_ERROR("Duplicate asset key '{}' -- keeping the first, ignoring the second. Keys drop the file extension, so two files in one directory whose names differ only by extension will collide", entry.key);
    }

    m_assets_by_key[entry.key] = entry;
    m_keys_by_id[entry.id()] = entry.key;
    return {};
}

auto AssetRegistry::register_sub_asset(AssetEntry const& parent, SubAssetDescriptor const& descriptor) -> Common::Expected<void>
{
    if (descriptor.name.empty()) {
        return OA_ERROR("Sub-asset of '{}' has an empty name", parent.key);
    }
    if (descriptor.name.contains(SUB_ASSET_SEPARATOR)) {
        return OA_ERROR("Sub-asset name '{}' of '{}' contains the reserved '{}' separator", descriptor.name, parent.key, SUB_ASSET_SEPARATOR);
    }

    auto const* source = std::get_if<LooseAssetEntry>(&parent.source);
    if (source == nullptr) {
        return OA_ERROR("Cannot derive sub-asset '{}': packed assets are not supported yet", descriptor.name);
    }
    if (source->sub_asset.has_value()) {
        return OA_ERROR("Cannot derive sub-asset '{}' from '{}', which is itself a sub-asset", descriptor.name, parent.key);
    }

    AssetEntry const entry {
        .sidecar = AssetSidecar(Common::UUID::derive(parent.id(), descriptor.name), descriptor.type),
        .key = sub_asset_key(parent.key, descriptor.name),
        .source = LooseAssetEntry { .path = source->path, .sub_asset = descriptor.name }
    };

    OA_LOG_TRACE(Log::Registry, "{} -> {} ({})", entry.key, entry.id(), to_string(entry.type()));
    return register_asset(entry);
}

auto AssetRegistry::key_to_id(std::string const& key) const -> std::optional<AssetID>
{
    auto const it = m_assets_by_key.find(key);
    if (it == m_assets_by_key.end()) {
        return std::nullopt;
    }
    return it->second.id();
}

auto AssetRegistry::resolve(AssetID id) const -> Common::Expected<AssetEntry>
{
    auto const key_it = m_keys_by_id.find(id);
    if (key_it == m_keys_by_id.end()) {
        return OA_ERROR("Asset with ID {} not found in asset registry", id);
    }

    auto const asset_it = m_assets_by_key.find(key_it->second);
    if (asset_it == m_assets_by_key.end()) {
        return OA_ERROR("Asset with ID {} not found in asset registry", id);
    }

    return asset_it->second;
}

auto AssetRegistry::resolve_key(std::filesystem::path path) const -> std::string
{
    path = std::filesystem::weakly_canonical(path);
    path = std::filesystem::relative(path, m_root_directory);
    path.replace_extension("");
    return path.generic_string();
}

auto AssetRegistry::sub_asset_key(std::string const& parent_key, std::string const& sub_asset_name) -> std::string
{
    return std::format("{}{}{}", parent_key, SUB_ASSET_SEPARATOR, sub_asset_name);
}

auto AssetRegistry::entries() const -> std::unordered_map<std::string, AssetEntry> const&
{
    return m_assets_by_key;
}

}
