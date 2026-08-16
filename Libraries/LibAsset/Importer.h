/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <algorithm>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <Common/Noncopyable.h>
#include <LibAsset/Asset.h>

namespace Asset {

class AssetRegistry;
class AssetSidecar;

struct SubAssetDescriptor {
    std::string name;
    AssetType type {};
};

struct ImportContext {
    std::filesystem::path path;
    AssetRegistry const* registry = nullptr;
    AssetSidecar const* sidecar = nullptr;
    std::optional<std::string> sub_asset = std::nullopt;
};

inline auto claims_extension(std::filesystem::path const& path, std::vector<std::string> const& extensions) -> bool
{
    auto const file_name = path.filename().string();
    return std::ranges::any_of(extensions, [&](auto const& extension) {
        return file_name.size() > extension.size() && file_name.ends_with(extension);
    });
}

}
