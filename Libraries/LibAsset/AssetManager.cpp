/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <format>

#include <Common/Expected.h>
#include <LibAsset/AssetManager.h>

namespace Asset {

AssetManager::AssetManager(Configuration const& config)
    : m_asset_registry(config.source_root)
    , m_cache_root(config.cache_root.empty() ? default_cache_root(config.source_root) : config.cache_root)
{
    if (m_cache_root.empty()) {
        OA_LOG_WARN(Log::Manager, "No cooked asset cache root could be resolved from '{}'", config.source_root.string());
        return;
    }
    OA_LOG_DEBUG(Log::Manager, "Source root '{}', cooked asset cache '{}'", config.source_root.string(), m_cache_root.string());
}

AssetManager::AssetManager(std::filesystem::path const& source_root)
    : AssetManager(Configuration { .source_root = source_root, .cache_root = {} })
{
}

auto AssetManager::default_cache_root(std::filesystem::path const& source_root) -> std::filesystem::path
{
    if (source_root.empty()) {
        return {};
    }

    auto normalized = std::filesystem::path(source_root).lexically_normal();
    if (!normalized.has_filename()) {
        normalized = normalized.parent_path();
    }

    return normalized.parent_path() / ".omnia" / "imported";
}

auto AssetManager::registry() const -> AssetRegistry const&
{
    return m_asset_registry;
}

auto AssetManager::cooked_asset_cache_root() const -> std::filesystem::path const&
{
    return m_cache_root;
}

auto AssetManager::cooked_asset_path(AssetID id) const -> std::filesystem::path
{
    return m_cache_root / std::format("{}{}", id.to_string(), COOKED_FILE_EXTENSION);
}

auto AssetManager::load_loose_assets() -> Common::Expected<void>
{
    return m_asset_registry.scan();
}

auto AssetManager::load_packed_assets() -> Common::Expected<void>
{
    OA_LOG_DEBUG(Log::Manager, "Packed assets are not implemented yet");
    return {};
}

}
