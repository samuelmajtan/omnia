/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <format>

#include <Common/Expected.h>
#include <LibAsset/AssetFile.h>

namespace Asset {

void AssetFileHeader::write(Binary::ByteWriter& writer) const
{
    writer.write<u64>(MAGIC);
    writer.write<u32>(format_version);
    writer.write<u8>(static_cast<u8>(asset_type));
    writer.write<u32>(importer_version);
    writer.write<u8>(static_cast<u8>(compression));
    writer.write<u64>(source_hash);
    writer.write<u64>(payload_size);
}

auto AssetFileHeader::read(Binary::ByteReader& reader) -> Common::Expected<AssetFileHeader>
{
    u64 file_magic {};
    TRY_ASSIGN(file_magic, reader.read<u64>());
    if (file_magic != MAGIC) {
        return OA_ERROR("Not an Omnia asset file (bad magic)");
    }

    u32 format_version {};
    TRY_ASSIGN(format_version, reader.read<u32>());
    if (format_version != VERSION) {
        return OA_ERROR("Cooked with asset format version {}, this build is {}", format_version, VERSION);
    }

    AssetType asset_type {};
    TRY_ASSIGN(asset_type, reader.read_enum<AssetType>(AssetType::Model, AssetType::Shader));

    u32 importer_version {};
    TRY_ASSIGN(importer_version, reader.read<u32>());

    CompressionMode compression {};
    TRY_ASSIGN(compression, reader.read_enum<CompressionMode>(CompressionMode::None, CompressionMode::None));

    u64 source_hash {};
    TRY_ASSIGN(source_hash, reader.read<u64>());

    u64 payload_size {};
    TRY_ASSIGN(payload_size, reader.read<u64>());
    if (payload_size > reader.remaining()) {
        return OA_ERROR("Truncated asset file: header declares a {} byte payload, {} bytes present", payload_size, reader.remaining());
    }

    return AssetFileHeader {
        .format_version = format_version,
        .asset_type = asset_type,
        .importer_version = importer_version,
        .source_hash = source_hash,
        .compression = compression,
        .payload_size = payload_size
    };
}

auto AssetFileHeader::matches(AssetType type, u32 expected_importer_version, u64 expected_source_hash) const -> bool
{
    return asset_type == type
        && importer_version == expected_importer_version
        && source_hash == expected_source_hash;
}

}
