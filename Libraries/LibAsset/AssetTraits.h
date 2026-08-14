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
#include <Common/Types.h>
#include <LibAsset/Asset.h>
#include <LibAsset/Export.h>
#include <LibAsset/Importer.h>
#include <LibAsset/ModelImporter.h>
#include <LibAsset/ShaderImporter.h>
#include <LibAsset/TextureImporter.h>

namespace Asset {

template<typename T>
struct AssetTraits;

template<>
struct ASSET_API AssetTraits<ShaderData> {
    using Importer = ShaderImporter;

    static constexpr auto TYPE = AssetType::Shader;
    static constexpr auto VERSION = ShaderImporter::VERSION;

    static auto extensions() -> std::vector<std::string>;
    static auto import(ImportContext const& context) -> std::expected<ShaderData, std::string>;
    static auto source_hash(ImportContext const& context) -> std::expected<u64, std::string>;
    static void write(Binary::ByteWriter& writer, ShaderData const& data);
    static auto read(Binary::ByteReader& reader) -> std::expected<ShaderData, std::string>;
};

template<>
struct ASSET_API AssetTraits<TextureData> {
    using Importer = TextureImporter;

    static constexpr auto TYPE = AssetType::Texture;
    static constexpr auto VERSION = TextureImporter::VERSION;

    static auto extensions() -> std::vector<std::string>;
    static auto import(ImportContext const& context) -> std::expected<TextureData, std::string>;
    static auto source_hash(ImportContext const& context) -> std::expected<u64, std::string>;
    static void write(Binary::ByteWriter& writer, TextureData const& data);
    static auto read(Binary::ByteReader& reader) -> std::expected<TextureData, std::string>;
};

template<>
struct ASSET_API AssetTraits<ModelData> {
    using Importer = ModelImporter;

    static constexpr auto TYPE = AssetType::Model;
    static constexpr auto VERSION = ModelImporter::VERSION;

    static auto extensions() -> std::vector<std::string>;
    static auto import(ImportContext const& context) -> std::expected<ModelData, std::string>;
    static auto source_hash(ImportContext const& context) -> std::expected<u64, std::string>;
    static void write(Binary::ByteWriter& writer, ModelData const& data);
    static auto read(Binary::ByteReader& reader) -> std::expected<ModelData, std::string>;
};

template<typename T>
concept AssetData = requires(ImportContext const& context, Binary::ByteWriter& writer, Binary::ByteReader& reader, T const& value) {
    { AssetTraits<T>::TYPE } -> std::convertible_to<AssetType>;
    { AssetTraits<T>::VERSION } -> std::convertible_to<u32>;
    { AssetTraits<T>::extensions() } -> std::same_as<std::vector<std::string>>;
    { AssetTraits<T>::import(context) } -> std::same_as<std::expected<T, std::string>>;
    { AssetTraits<T>::source_hash(context) } -> std::same_as<std::expected<u64, std::string>>;
    { AssetTraits<T>::write(writer, value) };
    { AssetTraits<T>::read(reader) } -> std::same_as<std::expected<T, std::string>>;
};

inline auto asset_type_for(std::filesystem::path const& path) -> std::optional<AssetType>
{
    if (claims_extension(path, AssetTraits<ModelData>::extensions())) {
        return AssetTraits<ModelData>::TYPE;
    }
    if (claims_extension(path, AssetTraits<TextureData>::extensions())) {
        return AssetTraits<TextureData>::TYPE;
    }
    if (claims_extension(path, AssetTraits<ShaderData>::extensions())) {
        return AssetTraits<ShaderData>::TYPE;
    }
    return std::nullopt;
}

}
