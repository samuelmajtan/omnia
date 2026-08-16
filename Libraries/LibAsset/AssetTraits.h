/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <concepts>
#include <expected>
#include <optional>
#include <string>
#include <vector>

#include <Common/ByteStream.h>
#include <Common/Expected.h>
#include <Common/Types.h>
#include <LibAsset/AnimationImporter.h>
#include <LibAsset/Asset.h>
#include <LibAsset/Export.h>
#include <LibAsset/Importer.h>
#include <LibAsset/ModelImporter.h>
#include <LibAsset/ShaderImporter.h>
#include <LibAsset/TextureImporter.h>

namespace Asset {

template<typename T>
struct AssetTraits;

template<typename T, typename ImporterType, AssetType ASSET_TYPE>
struct ImporterTraits {
    using Importer = ImporterType;

    static constexpr auto TYPE = ASSET_TYPE;
    static constexpr auto VERSION = ImporterType::VERSION;

    static auto extensions() -> std::vector<std::string>
    {
        return Importer::supported_extensions();
    }

    static auto import(ImportContext const& context) -> Common::Expected<T>
    {
        return Importer::import(context);
    }

    static auto source_hash(ImportContext const& context) -> Common::Expected<u64>
    {
        return Importer::source_hash(context);
    }

    static auto enumerate_sub_assets(std::filesystem::path const& path) -> Common::Expected<std::vector<SubAssetDescriptor>>
    {
        if constexpr (requires { Importer::enumerate_sub_assets(path); }) {
            return Importer::enumerate_sub_assets(path);
        } else {
            return std::vector<SubAssetDescriptor> {};
        }
    }
};

template<>
struct ASSET_API AssetTraits<ShaderData> : ImporterTraits<ShaderData, ShaderImporter, AssetType::Shader> {
    static void write(Binary::ByteWriter& writer, ShaderData const& data);
    static auto read(Binary::ByteReader& reader) -> Common::Expected<ShaderData>;
};

template<>
struct ASSET_API AssetTraits<TextureData> : ImporterTraits<TextureData, TextureImporter, AssetType::Texture> {
    static void write(Binary::ByteWriter& writer, TextureData const& data);
    static auto read(Binary::ByteReader& reader) -> Common::Expected<TextureData>;
};

template<>
struct ASSET_API AssetTraits<ModelData> : ImporterTraits<ModelData, ModelImporter, AssetType::Model> {
    static void write(Binary::ByteWriter& writer, ModelData const& data);
    static auto read(Binary::ByteReader& reader) -> Common::Expected<ModelData>;
};

template<>
struct ASSET_API AssetTraits<AnimationData> : ImporterTraits<AnimationData, AnimationImporter, AssetType::Animation> {
    static void write(Binary::ByteWriter& writer, AnimationData const& data);
    static auto read(Binary::ByteReader& reader) -> Common::Expected<AnimationData>;
};

template<typename T>
concept AssetData = requires(ImportContext const& context, Binary::ByteWriter& writer, Binary::ByteReader& reader, T const& value) {
    { AssetTraits<T>::TYPE } -> std::convertible_to<AssetType>;
    { AssetTraits<T>::VERSION } -> std::convertible_to<u32>;
    { AssetTraits<T>::extensions() } -> std::same_as<std::vector<std::string>>;
    { AssetTraits<T>::import(context) } -> std::same_as<Common::Expected<T>>;
    { AssetTraits<T>::source_hash(context) } -> std::same_as<Common::Expected<u64>>;
    { AssetTraits<T>::write(writer, value) };
    { AssetTraits<T>::read(reader) } -> std::same_as<Common::Expected<T>>;
    { AssetTraits<T>::enumerate_sub_assets(std::filesystem::path {}) } -> std::same_as<Common::Expected<std::vector<SubAssetDescriptor>>>;
};

ASSET_API auto asset_type_for(std::filesystem::path const& path) -> std::optional<AssetType>;
ASSET_API auto sub_assets_for(AssetType type, std::filesystem::path const& path) -> Common::Expected<std::vector<SubAssetDescriptor>>;

}
