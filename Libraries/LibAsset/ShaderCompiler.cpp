/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <array>
#include <shaderc/shaderc.hpp>

#include <Common/Expected.h>
#include <Common/File.h>
#include <LibAsset/ShaderCompiler.h>

namespace Asset::ShaderCompiler {

static auto to_shaderc(Graphics::ShaderStage stage) -> shaderc_shader_kind
{
    switch (stage) {
        using Graphics::ShaderStage;
    case ShaderStage::Vertex:
        return shaderc_vertex_shader;
    case ShaderStage::Fragment:
        return shaderc_fragment_shader;
    }
    return shaderc_glsl_infer_from_source;
}

class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface {
public:
    explicit ShaderIncluder(std::filesystem::path const& base_path)
        : m_base_path(base_path)
    {
    }

    auto GetInclude(char const* requested_source, [[maybe_unused]] shaderc_include_type type, char const* requesting_source, [[maybe_unused]] size_t include_depth) -> shaderc_include_result* override
    {
        auto* result = new shaderc_include_result;
        auto* data = new std::string[2];

        auto content = File::read_all(m_base_path / requested_source);
        if (content.has_value()) {
            data[0] = requesting_source;
            data[1] = content.value();
        } else {
            data[0] = "";
            data[1] = std::string(content.error().message());
        }
        result->source_name = data[0].c_str();
        result->source_name_length = data[0].size();
        result->content = data[1].c_str();
        result->content_length = data[1].size();
        result->user_data = data;

        return result;
    }

    void ReleaseInclude(shaderc_include_result* data) override
    {
        delete[] static_cast<std::string*>(data->user_data);
        delete data;
    }
private:
    std::filesystem::path m_base_path;
};

auto compile_spirv(std::filesystem::path const& shader_path, std::string_view glsl_source, Graphics::ShaderStage stage) -> Common::Expected<std::vector<u8>>
{
    shaderc::Compiler const compiler;
    shaderc::CompileOptions options;
    options.SetIncluder(std::make_unique<ShaderIncluder>(shader_path.parent_path()));
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    auto const result = compiler.CompileGlslToSpv(glsl_source.data(), glsl_source.size(), to_shaderc(stage), shader_path.filename().string().c_str(), options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        return OA_ERROR("Failed to compile shader '{}': {}", shader_path.string(), result.GetErrorMessage());
    }

    std::vector<u32> temp(result.cbegin(), result.cend());
    return std::vector<u8>(reinterpret_cast<u8*>(temp.data()), reinterpret_cast<u8*>(temp.data() + temp.size()));
}

}
