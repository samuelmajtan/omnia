/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <set>

#include <Common/Expected.h>

#include <Common/File.h>
#include <Common/Hash.h>
#include <Common/String.h>
#include <LibAsset/ShaderCompiler.h>
#include <LibAsset/ShaderImporter.h>
#include <LibDebug/Logger.h>

namespace Asset {

namespace {

constexpr Debug::Logger Logger("Shader Importer");

// Extract the path of included file from a line, TODO: Seems like this is not very robust, use a regex?
auto included_path(std::string_view line) -> std::optional<std::string_view>
{
    auto const trimmed = String::trimmed_left(line);
    if (!trimmed.starts_with("#include")) {
        return std::nullopt;
    }

    auto const open = trimmed.find_first_of("<\"");
    if (open == std::string_view::npos) {
        return std::nullopt;
    }

    auto const close = trimmed.find(trimmed[open] == '<' ? '>' : '"', open + 1);
    if (close == std::string_view::npos) {
        return std::nullopt;
    }

    return trimmed.substr(open + 1, close - open - 1);
}

void collect_includes(std::filesystem::path const& base, std::filesystem::path const& file, std::set<std::filesystem::path>& visited)
{
    auto const lines = File::read_lines(file);
    if (!lines.has_value()) {
        Logger.warn("Ignoring {} while hashing shader includes: {}", file.string(), lines.error());
        return;
    }

    for (auto const& line : lines.value()) {
        auto const target = included_path(line);
        if (!target) {
            continue;
        }

        auto resolved = (base / target.value()).lexically_normal();
        std::error_code error;
        if (!std::filesystem::exists(resolved, error) || error) {
            Logger.warn("Ignoring include '{}' of {}: it does not exist, so changes to it will not invalidate the cooked shader", target.value(), file.string());
            continue;
        }

        if (visited.insert(resolved).second) {
            collect_includes(base, resolved, visited);
        }
    }
}

}

auto ShaderImporter::import(ImportContext const& context) -> Common::Expected<ShaderData>
{
    auto const& path = context.path;
    if (!std::filesystem::exists(path)) {
        return OA_ERROR("Shader file '{}' does not exist", path.string());
    }

    if (!claims_extension(path, supported_extensions())) {
        return OA_ERROR("'{}' is not a shader: expected a .vs.glsl or .fs.glsl suffix", path.filename().string());
    }

    auto file_name = path.stem().string();
    auto shader_stage_string = std::string_view(file_name).substr(file_name.find_last_of('.') + 1);
    Graphics::ShaderStage shader_stage {};
    if (shader_stage_string == "vs") {
        shader_stage = Graphics::ShaderStage::Vertex;
    } else if (shader_stage_string == "fs") {
        shader_stage = Graphics::ShaderStage::Fragment;
    } else {
        return OA_ERROR("Unsupported shader stage '{}'", shader_stage_string);
    }

    ShaderData shader_data;
    shader_data.stage = shader_stage;

    std::string file_content;
    TRY_ASSIGN(file_content, File::read_all(path));

    {
        Graphics::ShaderVariant spirv_variant;
        spirv_variant.format = Graphics::ShaderFormat::SPIRV;

        TRY_ASSIGN(spirv_variant.bytecode, ShaderCompiler::compile_spirv(path, file_content, shader_stage));
        shader_data.variants.push_back(spirv_variant);
    }

    return shader_data;
}

auto ShaderImporter::source_hash(ImportContext const& context) -> Common::Expected<u64>
{
    auto const& path = context.path;

    u64 hash {};
    TRY_ASSIGN(hash, File::hash_file(path));

    // Changing the contents of an included file should also change the hash of the top-level shader.
    std::set<std::filesystem::path> includes;
    collect_includes(path.parent_path(), path, includes);

    for (auto const& include : includes) {
        hash = Hash::fnv1a(include.generic_string(), hash);

        std::vector<std::byte> contents;
        TRY_ASSIGN(contents, File::read_binary(include));
        hash = Hash::fnv1a(contents, hash);
    }

    return hash;
}

auto ShaderImporter::supported_extensions() -> std::vector<std::string>
{
    return { ".vs.glsl", ".fs.glsl" };
}

}
