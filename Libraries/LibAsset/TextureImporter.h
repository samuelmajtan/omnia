/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Common/Expected.h>
#include <LibAsset/Export.h>
#include <LibAsset/Importer.h>

namespace Asset {

enum class TextureColorSpace : u8 {
    Linear = 0,
    Srgb,
    Count
};

struct TextureData {
    u32 width {};
    u32 height {};
    TextureColorSpace color_space = TextureColorSpace::Linear;
    std::vector<u8> data;
};

class ASSET_API TextureImporter final {
public:
    static constexpr u32 VERSION = 1;

public:
    static auto import(ImportContext const& context) -> Common::Expected<TextureData>;
    static auto source_hash(ImportContext const& context) -> Common::Expected<u64>;
    static auto supported_extensions() -> std::vector<std::string>;
};

}
