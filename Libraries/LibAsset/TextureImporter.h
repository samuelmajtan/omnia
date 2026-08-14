/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibAsset/Export.h>
#include <LibAsset/Importer.h>

namespace Asset {

enum class ColorSpace : u8 {
    Linear = 0,
    Srgb
};

struct TextureData {
    u32 width {};
    u32 height {};
    ColorSpace color_space = ColorSpace::Linear;
    std::vector<u8> data;
};

class ASSET_API TextureImporter final {
public:
    static constexpr u32 VERSION = 1;

    static auto import(std::filesystem::path const& path) -> std::expected<TextureData, std::string>;
    static auto source_hash(std::filesystem::path const& path) -> std::expected<u64, std::string>;
    static auto supported_extensions() -> std::vector<std::string>;
};

template<>
struct ImporterTrait<TextureData> {
    using type = TextureImporter;
};

}
