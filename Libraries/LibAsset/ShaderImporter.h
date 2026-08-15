/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Common/Expected.h>
#include <LibAsset/Export.h>
#include <LibAsset/Importer.h>
#include <LibGraphics/ShaderTypes.h>

namespace Asset {

using ShaderData = Graphics::ShaderData;

class ASSET_API ShaderImporter final {
public:
    static constexpr u32 VERSION = 1;

public:
    static auto import(ImportContext const& context) -> Common::Expected<ShaderData>;
    static auto source_hash(ImportContext const& context) -> Common::Expected<u64>;
    static auto supported_extensions() -> std::vector<std::string>;
};

}
