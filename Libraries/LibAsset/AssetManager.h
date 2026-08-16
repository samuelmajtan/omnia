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
#include <Common/Time.h>
#include <LibAsset/AssetFile.h>
#include <LibAsset/AssetRegistry.h>
#include <LibAsset/AssetTraits.h>
#include <LibAsset/Export.h>
#include <LibAsset/Log.h>

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

    auto load_loose_assets() -> Common::Expected<void>;
    auto load_packed_assets() -> Common::Expected<void>;

    template<AssetData T>
    auto import(AssetID id) const -> Common::Expected<T>
    {
        auto entry = TRY(m_asset_registry.resolve(id));
        return import_entry<T>(entry, false);
    }
private:
    auto context_for(AssetEntry const& entry, LooseAssetEntry const& source) const -> ImportContext
    {
        return ImportContext {
            .path = source.path,
            .registry = &m_asset_registry,
            .sidecar = &entry.sidecar,
            .sub_asset = source.sub_asset
        };
    }

    template<AssetData T>
    auto import_entry(AssetEntry const& entry, bool force) const -> Common::Expected<T>
    {
        auto const* source = std::get_if<LooseAssetEntry>(&entry.source);
        if (source == nullptr) {
            return OA_ERROR("Packed assets are not supported yet");
        }

        auto const context = context_for(entry, *source);

        auto hash = TRY(AssetTraits<T>::source_hash(context));
        auto const source_hash = entry.sidecar.hash_settings(hash);

        if (!force) {
            if (auto cached = try_load_cooked_asset<T>(entry, source_hash); cached.has_value()) {
                OA_LOG_TRACE(Log::Manager, "Cache hit for '{}' ({})", entry.key, to_string(entry.type()));
                return std::move(cached).value();
            }
        }

        OA_LOG_TRACE(Log::Manager, "Importing '{}' from {}", entry.key, source->path.string());

        Time::Stopwatch const stopwatch;
        auto value = TRY(AssetTraits<T>::import(context));
        TRY(write_cooked_asset<T>(entry, value, source_hash));

        OA_LOG_DEBUG(Log::Manager, "Imported '{}' ({}) in {:.1f}ms", entry.key, to_string(entry.type()), stopwatch.elapsed_milliseconds());
        return value;
    }

    template<AssetData T>
    auto try_load_cooked_asset(AssetEntry const& entry, u64 source_hash) const -> std::optional<T>
    {
        auto const contents = File::read_binary(cooked_asset_path(entry.id()));
        if (!contents.has_value()) {
            OA_LOG_TRACE(Log::Manager, "No cooked asset '{}', importing from source", entry.key);
            return std::nullopt;
        }

        Binary::ByteReader reader(contents.value());
        auto const header = AssetFileHeader::read(reader);
        if (!header.has_value()) {
            OA_LOG_WARN(Log::Manager, "Discarding the cooked asset '{}': {}", entry.key, header.error());
            return std::nullopt;
        }

        auto const& file_header = header.value();
        if (!file_header.matches(AssetTraits<T>::TYPE, AssetTraits<T>::VERSION, source_hash)) {
            if (file_header.asset_type != AssetTraits<T>::TYPE) {
                OA_LOG_DEBUG(Log::Manager, "Re-cooking '{}': the blob is {}, expected {}", entry.key, to_string(file_header.asset_type), to_string(AssetTraits<T>::TYPE));
            } else if (file_header.importer_version != AssetTraits<T>::VERSION) {
                OA_LOG_DEBUG(Log::Manager, "Re-cooking '{}': importer version {} -> {}", entry.key, file_header.importer_version, AssetTraits<T>::VERSION);
            } else {
                OA_LOG_DEBUG(Log::Manager, "Re-cooking '{}': the source or its import settings changed", entry.key);
            }
            return std::nullopt;
        }

        auto value = AssetTraits<T>::read(reader);
        if (!value.has_value()) {
            OA_LOG_WARN(Log::Manager, "The cooked asset '{}' is corrupt, re-importing: {}", entry.key, value.error());
            return std::nullopt;
        }
        return std::move(value).value();
    }

    template<AssetData T>
    auto write_cooked_asset(AssetEntry const& entry, T const& value, u64 source_hash) const -> Common::Expected<void>
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

        if (auto result = File::write_binary(cooked_asset_path(entry.id()), file.bytes()); !result.has_value()) {
            return OA_ERROR("Failed to write the cooked asset for '{}': {}", entry.key, result.error().message());
        }

        OA_LOG_TRACE(Log::Manager, "Cooked '{}' as {} ({} bytes)", entry.key, cooked_asset_path(entry.id()).string(), file.bytes().size());
        return {};
    }
private:
    AssetRegistry m_asset_registry;
    std::filesystem::path m_cache_root;
};

}
