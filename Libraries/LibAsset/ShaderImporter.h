/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibAsset/Export.h>
#include <LibAsset/Importer.h>
#include <LibGraphics/ShaderTypes.h>

namespace Asset {

using ShaderData = Graphics::ShaderData;

class ASSET_API ShaderImporter final {
public:
    static constexpr u32 VERSION = 1;

    static auto import(std::filesystem::path const& path) -> std::expected<ShaderData, std::string>;
    static auto source_hash(std::filesystem::path const& path) -> std::expected<u64, std::string>;
    static auto supported_extensions() -> std::vector<std::string>;
};

template<>
struct ImporterTrait<ShaderData> {
    using type = ShaderImporter;
};

}
