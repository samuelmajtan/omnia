/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <Common/Expected.h>
#include <Common/File.h>
#include <LibAsset/AssetSidecar.h>
#include <LibAsset/TextureImporter.h>

namespace Asset {

auto TextureImporter::import(ImportContext const& context) -> Common::Expected<TextureData>
{
    auto const& path = context.path;
    if (!std::filesystem::exists(path)) {
        return OA_ERROR("Texture file '{}' does not exist", path.string());
    }

    auto extension = path.extension().string();
    auto supported_extensions = TextureImporter::supported_extensions();
    if (std::ranges::find(supported_extensions.begin(), supported_extensions.end(), extension) == supported_extensions.end()) {
        return OA_ERROR("Unsupported texture file extension '{}'", extension);
    }

    std::vector<std::byte> file_content_value;
    TRY_ASSIGN(file_content_value, File::read_binary(path));

    i32 width = 0;
    i32 height = 0;
    i32 channels = 0;
    auto* data = stbi_load_from_memory(reinterpret_cast<stbi_uc const*>(file_content_value.data()), static_cast<i32>(file_content_value.size()), &width, &height, &channels, 4);
    if (data == nullptr) {
        return OA_ERROR("Failed to load texture from file '{}'", path.string());
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

auto TextureImporter::source_hash(ImportContext const& context) -> Common::Expected<u64>
{
    return File::hash_file(context.path);
}

auto TextureImporter::supported_extensions() -> std::vector<std::string>
{
    return { ".png", ".jpg", ".jpeg", ".bmp", ".tga" };
}

}
