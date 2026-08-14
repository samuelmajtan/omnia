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
#include <LibAsset/AssetSidecar.h>

namespace Asset {

namespace {

auto trimmed(std::string_view text) -> std::string_view
{
    auto const is_space = [](char character) {
        return character == ' ' || character == '\t' || character == '\r' || character == '\n';
    };

    while (!text.empty() && is_space(text.front())) {
        text.remove_prefix(1);
    }
    while (!text.empty() && is_space(text.back())) {
        text.remove_suffix(1);
    }
    return text;
}

constexpr auto uuid_key = "uuid";
constexpr auto type_key = "type";

}

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

auto AssetSidecar::load(std::filesystem::path const& sidecar_path) -> std::expected<AssetSidecar, std::string>
{
    std::vector<std::string> lines;
    TRY_ASSIGN(lines, File::read_lines(sidecar_path));

    AssetSidecar sidecar;
    std::optional<AssetID> id;
    std::optional<AssetType> type;

    for (auto const& [number, line] : std::views::enumerate(lines)) {
        auto const content = trimmed(line);
        if (content.empty() || content.starts_with('#')) {
            continue;
        }

        auto const separator = content.find('=');
        if (separator == std::string_view::npos) {
            return std::unexpected(std::format("{}:{}: expected 'key = value', got '{}'.", sidecar_path.string(), number + 1, content));
        }

        auto const key = std::string(trimmed(content.substr(0, separator)));
        auto value = std::string(trimmed(content.substr(separator + 1)));

        if (key == uuid_key) {
            id = Platform::UUID::from_string(value);
            if (!id) {
                return std::unexpected(std::format("{}:{}: '{}' is not a valid UUID.", sidecar_path.string(), number + 1, value));
            }
        } else if (key == type_key) {
            type = asset_type_from_string(value);
            if (!type) {
                return std::unexpected(std::format("{}:{}: '{}' is not a known asset type.", sidecar_path.string(), number + 1, value));
            }
        } else {
            sidecar.m_settings.emplace(key, std::move(value));
        }
    }

    if (!id) {
        return std::unexpected(std::format("{}: missing required '{}' key.", sidecar_path.string(), uuid_key));
    }
    if (!type) {
        return std::unexpected(std::format("{}: missing required '{}' key.", sidecar_path.string(), type_key));
    }

    sidecar.m_id = id.value();
    sidecar.m_type = type.value();
    return sidecar;
}

auto AssetSidecar::save(std::filesystem::path const& sidecar_path) const -> std::expected<void, std::string>
{
    auto const id_string = m_id.to_string();
    if (!id_string) {
        return std::unexpected(std::format("Cannot write {}: the asset ID could not be formatted.", sidecar_path.string()));
    }

    std::string content;
    content += SIDECAR_HEADER;
    content += std::format("{} = {}\n", uuid_key, id_string.value());
    content += std::format("{} = {}\n", type_key, to_string(m_type));

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
