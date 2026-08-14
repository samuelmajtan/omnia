/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <Common/File.h>
#include <LibAsset/AssetSidecar.h>
#include <LibAsset/TextureImporter.h>

namespace Asset {

auto TextureImporter::import(ImportContext const& context) -> std::expected<TextureData, std::string>
{
    auto const& path = context.path;
    if (!std::filesystem::exists(path)) {
        return std::unexpected(std::format("Texture file '{}' does not exist", path.string()));
    }

    auto extension = path.extension().string();
    auto supported_extensions = TextureImporter::supported_extensions();
    if (std::ranges::find(supported_extensions.begin(), supported_extensions.end(), extension) == supported_extensions.end()) {
        return std::unexpected(std::format("Unsupported texture file extension '{}'", extension));
    }

    auto file_content = File::read_binary(path);
    if (!file_content) {
        return std::unexpected(file_content.error());
    }
    auto file_content_value = file_content.value();

    i32 width = 0;
    i32 height = 0;
    i32 channels = 0;
    auto* data = stbi_load_from_memory(reinterpret_cast<stbi_uc const*>(file_content_value.data()), static_cast<i32>(file_content_value.size()), &width, &height, &channels, 4);
    if (data == nullptr) {
        return std::unexpected(std::format("Failed to load texture from file '{}'", path.string()));
    }
    auto const size = static_cast<std::size_t>(width) * height * 4;
    auto const is_srgb = context.sidecar != nullptr && context.sidecar->bool_setting("srgb", false);

    TextureData texture_data;
    texture_data.width = static_cast<u32>(width);
    texture_data.height = static_cast<u32>(height);
    texture_data.color_space = is_srgb ? TextureColorSpace::Srgb : TextureColorSpace::Linear;
    texture_data.data.resize(size);
    std::memcpy(texture_data.data.data(), data, size);
    stbi_image_free(data);
    return texture_data;
}

auto TextureImporter::source_hash(ImportContext const& context) -> std::expected<u64, std::string>
{
    return File::hash_file(context.path);
}

auto TextureImporter::supported_extensions() -> std::vector<std::string>
{
    return { ".png", ".jpg", ".jpeg", ".bmp", ".tga" };
}

}
