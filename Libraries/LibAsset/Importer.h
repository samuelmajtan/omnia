/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <algorithm>
#include <any>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

#include <Common/Noncopyable.h>

namespace Asset {

template<typename T>
struct ImporterTrait;

inline auto claims_extension(std::filesystem::path const& path, std::vector<std::string> const& extensions) -> bool
{
    auto const file_name = path.filename().string();
    return std::ranges::any_of(extensions, [&](auto const& extension) {
        return file_name.size() > extension.size() && file_name.ends_with(extension);
    });
}

}
