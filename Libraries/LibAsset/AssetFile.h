/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <expected>
#include <string>

#include <Common/ByteStream.h>
#include <Common/Expected.h>
#include <Common/Types.h>
#include <LibAsset/Asset.h>
#include <LibAsset/Export.h>

namespace Asset {

enum class CompressionMode : u8 {
    None = 0
};

struct ASSET_API AssetFileHeader {
    static constexpr u32 VERSION = 1;
    static constexpr u64 MAGIC = 0x41494E4D4F5F4141ULL;

    u32 format_version = VERSION;
    AssetType asset_type {};
    u32 importer_version {};
    u64 source_hash {};
    CompressionMode compression = CompressionMode::None;
    u64 payload_size {};

    void write(Binary::ByteWriter& writer) const;
    static auto read(Binary::ByteReader& reader) -> Common::Expected<AssetFileHeader>;

    auto matches(AssetType type, u32 expected_importer_version, u64 expected_source_hash) const -> bool;
};

}
