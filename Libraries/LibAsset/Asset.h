/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <optional>
#include <string_view>

#include <Common/Types.h>
#include <LibPlatform/UUID.h>

namespace Asset {

using AssetID = Platform::UUID;

enum class AssetType : u8 {
    Model = 0,
    Texture,
    Shader
};

inline constexpr auto to_string(AssetType type) -> std::string_view
{
    switch (type) {
    case AssetType::Model:
        return "Model";
    case AssetType::Texture:
        return "Texture";
    case AssetType::Shader:
        return "Shader";
    }
    return "Unknown";
}

inline constexpr auto asset_type_from_string(std::string_view name) -> std::optional<AssetType>
{
    if (name == "Model") {
        return AssetType::Model;
    }
    if (name == "Texture") {
        return AssetType::Texture;
    }
    if (name == "Shader") {
        return AssetType::Shader;
    }
    return std::nullopt;
}

}
