/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <optional>

#include <Common/ByteStream.h>
#include <Common/Expected.h>
#include <Common/File.h>
#include <LibAsset/AssetFile.h>
#include <LibAsset/AssetRegistry.h>
#include <LibAsset/AssetTraits.h>
#include <LibAsset/Export.h>

namespace Asset {

class ASSET_API AssetManager final {
    OA_MAKE_DEFAULT_CONSTRUCTIBLE(AssetManager);
    OA_MAKE_NONCOPYABLE(AssetManager);
    OA_MAKE_NONMOVABLE(AssetManager);

public:
    static constexpr std::string_view COOKED_FILE_EXTENSION = ".oasset";

    struct Configuration {
        std::filesystem::path source_root;
        std::filesystem::path cache_root;
    };

    explicit AssetManager(Configuration const& config);
    explicit AssetManager(std::filesystem::path const& source_root);

    static auto default_cache_root(std::filesystem::path const& source_root) -> std::filesystem::path;

    auto registry() const -> AssetRegistry const&;
    auto cooked_asset_cache_root() const -> std::filesystem::path const&;
    auto cooked_asset_path(AssetID id) const -> std::filesystem::path;

    auto load_loose_assets() -> std::expected<void, std::string>;
    auto load_packed_assets() -> std::expected<void, std::string>;

    template<AssetData T>
    auto import(std::string const& key) const -> std::expected<T, std::string>
    {
        auto id = m_asset_registry.key_to_id(key);
        if (!id) {
            return std::unexpected(std::format("Asset with key '{}' not found.", key));
        }
        return import <T>(id.value());
    }

    template<AssetData T>
    auto import(AssetID id) const -> std::expected<T, std::string>
    {
        AssetEntry entry;
        TRY_ASSIGN(entry, m_asset_registry.resolve(id));
        return import_entry<T>(entry, false);
    }

private:
    auto context_for(AssetEntry const& entry, std::filesystem::path const& path) const -> ImportContext
    {
        return ImportContext {
            .path = path,
            .registry = &m_asset_registry,
            .sidecar = &entry.sidecar
        };
    }

    template<AssetData T>
    auto import_entry(AssetEntry const& entry, bool force) const -> std::expected<T, std::string>
    {
        auto const* source = std::get_if<LooseAssetEntry>(&entry.source);
        if (source == nullptr) {
            return std::unexpected("Packed assets are not supported yet");
        }

        auto const context = context_for(entry, source->path);

        u64 source_hash {};
        TRY_ASSIGN(source_hash, AssetTraits<T>::source_hash(context));
        source_hash = entry.sidecar.hash_settings(source_hash);

        if (!force) {
            if (auto cached = try_load_cooked_asset<T>(entry, source_hash); cached.has_value()) {
                return std::move(cached).value();
            }
        }

        T value {};
        TRY_ASSIGN(value, AssetTraits<T>::import(context));

        if (auto result = write_cooked_asset<T>(entry, value, source_hash); !result.has_value()) {
            return std::unexpected(std::move(result).error());
        }
        return value;
    }

    template<AssetData T>
    auto try_load_cooked_asset(AssetEntry const& entry, u64 source_hash) const -> std::optional<T>
    {
        auto const contents = File::read_binary(cooked_asset_path(entry.id()));
        if (!contents.has_value()) {
            return std::nullopt;
        }

        Binary::ByteReader reader(contents.value());
        auto const header = AssetFileHeader::read(reader);
        if (!header.has_value()) {
            return std::nullopt;
        }

        if (!header.value().matches(AssetTraits<T>::TYPE, AssetTraits<T>::VERSION, source_hash)) {
            return std::nullopt;
        }

        auto value = AssetTraits<T>::read(reader);
        if (!value.has_value()) {
            return std::nullopt;
        }
        return std::move(value).value();
    }

    template<AssetData T>
    auto write_cooked_asset(AssetEntry const& entry, T const& value, u64 source_hash) const -> std::expected<void, std::string>
    {
        Binary::ByteWriter payload;
        AssetTraits<T>::write(payload, value);

        AssetFileHeader const header {
            .asset_type = AssetTraits<T>::TYPE,
            .importer_version = AssetTraits<T>::VERSION,
            .source_hash = source_hash,
            .payload_size = payload.size()
        };

        Binary::ByteWriter file;
        header.write(file);
        file.write_bytes(payload.bytes());

        return File::write_binary(cooked_asset_path(entry.id()), file.bytes());
    }
private:
    AssetRegistry m_asset_registry;
    std::filesystem::path m_cache_root;
};

}
