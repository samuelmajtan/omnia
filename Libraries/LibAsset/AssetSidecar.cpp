/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <algorithm>
#include <format>
#include <ranges>

#include <Common/Expected.h>
#include <Common/File.h>
#include <Common/Hash.h>
#include <Common/String.h>
#include <LibAsset/AssetSidecar.h>

namespace Asset {

static constexpr auto UUID_KEY = "uuid";
static constexpr auto TYPE_KEY = "type";

AssetSidecar::AssetSidecar(AssetID id, AssetType type)
    : m_id(id)
    , m_type(type)
{
}

auto AssetSidecar::path_for(std::filesystem::path const& source_path) -> std::filesystem::path
{
    auto path = source_path;
    path += SIDECAR_EXTENSION;
    return path;
}

auto AssetSidecar::load(std::filesystem::path const& sidecar_path) -> Common::Expected<AssetSidecar>
{
    auto const lines = TRY(File::read_lines(sidecar_path));

    AssetSidecar sidecar;
    std::optional<AssetID> id;
    std::optional<AssetType> type;

    for (auto const& [number, line] : std::views::enumerate(lines)) {
        auto const content = String::trimmed(line);
        if (content.empty() || content.starts_with('#')) {
            continue;
        }

        auto const separator = content.find('=');
        if (separator == std::string_view::npos) {
            return OA_ERROR("{}:{}: expected 'key = value', got '{}'", sidecar_path.string(), number + 1, content);
        }

        auto const key = std::string(String::trimmed(content.substr(0, separator)));
        auto value = std::string(String::trimmed(content.substr(separator + 1)));

        if (key == UUID_KEY) {
            id = Common::UUID::from_string(value);
            if (!id) {
                return OA_ERROR("{}:{}: '{}' is not a valid UUID", sidecar_path.string(), number + 1, value);
            }
        } else if (key == TYPE_KEY) {
            type = asset_type_from_string(value);
            if (!type) {
                return OA_ERROR("{}:{}: '{}' is not a known asset type", sidecar_path.string(), number + 1, value);
            }
        } else {
            sidecar.m_settings.emplace(key, std::move(value));
        }
    }

    if (!id) {
        return OA_ERROR("{}: missing required '{}' key", sidecar_path.string(), UUID_KEY);
    }
    if (!type) {
        return OA_ERROR("{}: missing required '{}' key", sidecar_path.string(), TYPE_KEY);
    }

    sidecar.m_id = id.value();
    sidecar.m_type = type.value();
    return sidecar;
}

auto AssetSidecar::save(std::filesystem::path const& sidecar_path) const -> Common::Expected<void>
{
    std::string content;
    content += SIDECAR_HEADER;
    content += std::format("{} = {}\n", UUID_KEY, m_id.to_string());
    content += std::format("{} = {}\n", TYPE_KEY, to_string(m_type));

    std::vector<std::string> keys;
    keys.reserve(m_settings.size());
    for (auto const& [key, value] : m_settings) {
        keys.push_back(key);
    }
    std::ranges::sort(keys);

    for (auto const& key : keys) {
        content += std::format("{} = {}\n", key, m_settings.at(key));
    }

    return File::write_all(sidecar_path, content);
}

auto AssetSidecar::id() const -> AssetID
{
    return m_id;
}

auto AssetSidecar::type() const -> AssetType
{
    return m_type;
}

auto AssetSidecar::setting(std::string const& key) const -> std::optional<std::string>
{
    auto const it = m_settings.find(key);
    if (it == m_settings.end()) {
        return std::nullopt;
    }
    return it->second;
}

auto AssetSidecar::bool_setting(std::string const& key, bool fallback) const -> bool
{
    auto const value = setting(key);
    if (!value) {
        return fallback;
    }
    return value.value() == "true" || value.value() == "1";
}

void AssetSidecar::set_setting(std::string const& key, std::string value)
{
    m_settings[key] = std::move(value);
}

auto AssetSidecar::hash_settings(u64 seed) const -> u64
{
    std::vector<std::string> keys;
    keys.reserve(m_settings.size());
    for (auto const& [key, value] : m_settings) {
        if (key == NAME_SETTING) {
            continue;
        }
        keys.push_back(key);
    }
    std::ranges::sort(keys);

    auto hash = seed;
    for (auto const& key : keys) {
        hash = Hash::fnv1a(key, hash);
        hash = Hash::fnv1a(std::string_view("="), hash);
        hash = Hash::fnv1a(m_settings.at(key), hash);
        hash = Hash::fnv1a(std::string_view("\n"), hash);
    }
    return hash;
}

}
