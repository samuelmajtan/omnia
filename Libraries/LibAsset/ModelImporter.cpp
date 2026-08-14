/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <unordered_map>

#include <Common/File.h>
#include <LibAsset/ModelImporter.h>

namespace Asset {

auto ModelImporter::import(std::filesystem::path const& path, AssetRegistry const& asset_registry) -> std::expected<ModelData, std::string>
{
    if (!std::filesystem::exists(path)) {
        return std::unexpected(std::format("Model file '{}' does not exist", path.string()));
    }

    auto extension = path.extension().string();
    if (extension == ".gltf") {
        return import_gltf(path, asset_registry);
    }

    return std::unexpected(std::format("Unsupported model file extension '{}'", extension));
}

auto ModelImporter::source_hash(std::filesystem::path const& path) -> std::expected<u64, std::string>
{
    return File::hash_file(path);
}

auto ModelImporter::supported_extensions() -> std::vector<std::string>
{
    return { ".gltf" };
}

auto ModelImporter::import_gltf(std::filesystem::path const& path, AssetRegistry const& asset_registry) -> std::expected<ModelData, std::string>
{
    auto file_data = File::read_all(path);
    if (!file_data) {
        return std::unexpected(std::move(file_data).error());
    }

    cgltf_options const options {};
    cgltf_data* data = nullptr;
    if (cgltf_parse(&options, file_data->data(), file_data->size(), &data) != cgltf_result_success) {
        return std::unexpected(std::format("Failed to parse glTF file '{}'", path.string()));
    }
    if (cgltf_load_buffers(&options, data, path.string().c_str()) != cgltf_result_success) {
        cgltf_free(data);
        return std::unexpected(std::format("Failed to load buffers for glTF file '{}'", path.string()));
    }
    if (cgltf_validate(data) != cgltf_result_success) {
        cgltf_free(data);
        return std::unexpected(std::format("Invalid glTF file '{}'", path.string()));
    }

    ModelData model_data;

    for (cgltf_size i = 0; i < data->materials_count; ++i) {
        auto const& gltf_material = data->materials[i];
        auto& material = model_data.materials.emplace_back();
        auto& material_parameters = material.parameters;

        material.name = gltf_material.name ? gltf_material.name : std::format("Material_{}", i);

        auto resolve_texture_id = [&](cgltf_texture_view const& texture_view) -> std::optional<AssetID> {
            if (texture_view.texture == nullptr || texture_view.texture->image == nullptr) {
                return std::nullopt;
            }
            auto const texture_path = path.parent_path() / texture_view.texture->image->uri;
            auto key = asset_registry.resolve_key(texture_path);
            return asset_registry.key_to_id(key);
        };

        if (gltf_material.has_pbr_metallic_roughness) {
            auto const& pbr = gltf_material.pbr_metallic_roughness;

            material_parameters.base_color = Math::Vec4f(pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2], pbr.base_color_factor[3]);
            material.albedo_texture_id = resolve_texture_id(pbr.base_color_texture);

            material_parameters.metallic_factor = pbr.metallic_factor;
            material_parameters.roughness_factor = pbr.roughness_factor;
            material.metallic_roughness_texture_id = resolve_texture_id(pbr.metallic_roughness_texture);
        }

        material_parameters.normal_scale = gltf_material.normal_texture.scale;
        material.normal_texture_id = resolve_texture_id(gltf_material.normal_texture);

        material_parameters.occlusion_strength = gltf_material.occlusion_texture.scale;
        material.occlusion_texture_id = resolve_texture_id(gltf_material.occlusion_texture);

        material_parameters.emissive_factor = Math::Vec3f(gltf_material.emissive_factor[0], gltf_material.emissive_factor[1], gltf_material.emissive_factor[2]);
        material.emissive_texture_id = resolve_texture_id(gltf_material.emissive_texture);
    }

    auto get_material_index = [&](cgltf_material const* gltf_mat) -> u64 {
        if (gltf_mat == nullptr) {
            return 0;
        }

        for (cgltf_size i = 0; i < data->materials_count; ++i) {
            if (&data->materials[i] == gltf_mat) {
                return i;
            }
        }
        return 0;
    };

    for (cgltf_size i = 0; i < data->meshes_count; ++i) {
        auto const& gltf_mesh = data->meshes[i];

        for (cgltf_size j = 0; j < gltf_mesh.primitives_count; ++j) {
            auto const& primitive = gltf_mesh.primitives[j];
            if (primitive.type != cgltf_primitive_type_triangles) {
                continue;
            }

            auto& sub_mesh = model_data.sub_meshes.emplace_back();
            sub_mesh.material_index = get_material_index(primitive.material);

            cgltf_accessor const* position_accessor = nullptr;
            cgltf_accessor const* normal_accessor = nullptr;
            cgltf_accessor const* tex_coord_accessor = nullptr;
            cgltf_accessor const* tangent_accessor = nullptr;

            for (cgltf_size k = 0; k < primitive.attributes_count; ++k) {
                auto const& attribute = primitive.attributes[k];
                switch (attribute.type) {
                case cgltf_attribute_type_position:
                    position_accessor = attribute.data;
                    break;
                case cgltf_attribute_type_normal:
                    normal_accessor = attribute.data;
                    break;
                case cgltf_attribute_type_texcoord:
                    tex_coord_accessor = attribute.data;
                    break;
                case cgltf_attribute_type_tangent:
                    tangent_accessor = attribute.data;
                    break;
                case cgltf_attribute_type_joints:
                    break;
                case cgltf_attribute_type_weights:
                    break;
                default:
                    break;
                }
            }

            if (position_accessor == nullptr) {
                continue;
            }

            auto vertex_count = position_accessor->count;
            sub_mesh.vertices.reserve(vertex_count);

            for (cgltf_size vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
                Graphics::Vertex vertex {};

                if (position_accessor != nullptr) {
                    cgltf_accessor_read_float(position_accessor, vertex_index, &vertex.position.x, 3);
                }

                if (normal_accessor != nullptr) {
                    cgltf_accessor_read_float(normal_accessor, vertex_index, &vertex.normal.x, 3);
                }

                if (tex_coord_accessor != nullptr) {
                    cgltf_accessor_read_float(tex_coord_accessor, vertex_index, &vertex.tex_coord.x, 2);
                }

                if (tangent_accessor != nullptr) {
                    cgltf_accessor_read_float(tangent_accessor, vertex_index, &vertex.tangent.x, 4);
                }

                sub_mesh.vertices.push_back(vertex);
            }

            if (primitive.indices != nullptr) {
                auto index_count = primitive.indices->count;
                sub_mesh.indices.reserve(index_count);

                for (cgltf_size idx = 0; idx < index_count; ++idx) {
                    u32 index_value = 0;
                    cgltf_accessor_read_uint(primitive.indices, idx, &index_value, 1);
                    sub_mesh.indices.push_back(index_value);
                }
            } else {
                sub_mesh.indices.reserve(vertex_count);
                for (u32 idx = 0; idx < vertex_count; ++idx) {
                    sub_mesh.indices.push_back(idx);
                }
            }

            if (tangent_accessor == nullptr) {
                std::vector<Math::Vec3f> tangents(vertex_count);
                std::vector<Math::Vec3f> bitangents(vertex_count);

                for (std::size_t index = 0; index + 2 < sub_mesh.indices.size(); index += 3) {
                    auto const i0 = sub_mesh.indices[index + 0];
                    auto const i1 = sub_mesh.indices[index + 1];
                    auto const i2 = sub_mesh.indices[index + 2];

                    auto const& v0 = sub_mesh.vertices[i0];
                    auto const& v1 = sub_mesh.vertices[i1];
                    auto const& v2 = sub_mesh.vertices[i2];

                    auto const edge1 = v1.position - v0.position;
                    auto const edge2 = v2.position - v0.position;

                    auto const delta_uv1 = v1.tex_coord - v0.tex_coord;
                    auto const delta_uv2 = v2.tex_coord - v0.tex_coord;

                    auto denominator = (delta_uv1.x * delta_uv2.y) - (delta_uv2.x * delta_uv1.y);
                    if (std::abs(denominator) < 1e-6F) {
                        continue;
                    }

                    auto const tangent = ((edge1 * delta_uv2.y) - (edge2 * delta_uv1.y)) / denominator;
                    auto const bitangent = ((edge2 * delta_uv1.x) - (edge1 * delta_uv2.x)) / denominator;

                    tangents[i0] += tangent;
                    tangents[i1] += tangent;
                    tangents[i2] += tangent;

                    bitangents[i0] += bitangent;
                    bitangents[i1] += bitangent;
                    bitangents[i2] += bitangent;
                }

                for (std::size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
                    auto& vertex = sub_mesh.vertices[vertex_index];
                    auto& normal = vertex.normal;
                    auto& tangent = tangents[vertex_index];
                    auto& bitangent = bitangents[vertex_index];

                    auto t = tangent - normal * Math::dot(normal, tangent);
                    t.normalize();

                    auto handedness = (Math::dot(Math::cross(normal, t), bitangent) < 0.0F) ? -1.0F : 1.0F;

                    vertex.tangent = Math::Vec4f(t.x, t.y, t.z, handedness);
                }
            }
        }
    }

    std::erase_if(model_data.sub_meshes, [](auto const& submesh) {
        return submesh.indices.empty();
    });

    cgltf_free(data);
    return model_data;
}

}
