/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <Common/Expected.h>
#include <Common/Hash.h>
#include <Common/Platform.h>
#include <Common/Types.h>

namespace File {

inline auto read_all(std::filesystem::path const& path) -> Common::Expected<std::string>
{
    if (!std::filesystem::exists(path)) {
        return OA_ERROR("File does not exist: {}", path.string());
    }

    if (!std::filesystem::is_regular_file(path)) {
        return OA_ERROR("Path is not a regular file: {}", path.string());
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return OA_ERROR("Failed to open file: {}", path.string());
    }

    return std::string(std::istreambuf_iterator<char>(file), {});
}

inline auto read_lines(std::filesystem::path const& path) -> Common::Expected<std::vector<std::string>>
{
    if (!std::filesystem::exists(path)) {
        return OA_ERROR("File does not exist: {}", path.string());
    }

    if (!std::filesystem::is_regular_file(path)) {
        return OA_ERROR("Path is not a regular file: {}", path.string());
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return OA_ERROR("Failed to open file: {}", path.string());
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    return lines;
}

template<typename T = std::byte>
inline auto read_binary(std::filesystem::path const& path) -> Common::Expected<std::vector<T>>
{
    if (!std::filesystem::exists(path)) {
        return OA_ERROR("File does not exist: {}", path.string());
    }

    if (!std::filesystem::is_regular_file(path)) {
        return OA_ERROR("Path is not a regular file: {}", path.string());
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return OA_ERROR("Failed to open file: {}", path.string());
    }

    auto file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<T> data(file_size / sizeof(T));
    if (!file.read(reinterpret_cast<char*>(data.data()), file_size)) {
        return OA_ERROR("Failed to read file: {}", path.string());
    }

    return data;
}

inline auto create_parent_dir(std::filesystem::path const& path) -> Common::Expected<void>
{
    auto const parent = path.parent_path();
    if (parent.empty()) {
        return {};
    }

    std::error_code error;
    std::filesystem::create_directories(parent, error);
    if (error) {
        return OA_ERROR("Failed to create directory {}: {}", parent.string(), error.message());
    }
    return {};
}

inline auto write_all(std::filesystem::path const& path, std::string_view content) -> Common::Expected<void>
{
    TRY(create_parent_dir(path));

    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        return OA_ERROR("Failed to open file for writing: {}", path.string());
    }

    if (!file.write(content.data(), static_cast<std::streamsize>(content.size()))) {
        return OA_ERROR("Failed to write file: {}", path.string());
    }
    return {};
}

inline auto write_binary(std::filesystem::path const& path, std::span<std::byte const> data) -> Common::Expected<void>
{
    TRY(create_parent_dir(path));

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return OA_ERROR("Failed to open file for writing: {}", path.string());
    }

    if (!data.empty() && !file.write(reinterpret_cast<char const*>(data.data()), static_cast<std::streamsize>(data.size()))) {
        return OA_ERROR("Failed to write file: {}", path.string());
    }
    return {};
}

inline auto hash_file(std::filesystem::path const& path) -> Common::Expected<u64>
{
    auto const contents = TRY(File::read_binary(path));
    return Hash::fnv1a(contents);
}

}
