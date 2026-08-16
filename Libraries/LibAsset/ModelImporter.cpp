/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Common/Expected.h>
#include <Common/File.h>
#include <Common/Time.h>
#include <LibAsset/Log.h>
#include <LibAsset/ModelImporter.h>
#include <LibAsset/glTF.h>

namespace Asset {

namespace {

auto import_skeleton(cgltf_data const* data, std::string_view file_name) -> std::optional<Graphics::SkeletonData>
{
    if (data->skins_count == 0) {
        return std::nullopt;
    }
    if (data->skins_count > 1) {
        OA_LOG_WARN(Log::Model, "{}: {} skins found, only the first is imported", file_name, data->skins_count);
    }

    auto const& skin = data->skins[0];
    auto [nodes, indices] = glTF::flatten_node_hierarchy(data);
    if (nodes.size() != data->nodes_count) {
        OA_LOG_WARN(Log::Model, "{}: reached {} of {} nodes, the hierarchy has a cycle", file_name, nodes.size(), data->nodes_count);
        return std::nullopt;
    }

    Graphics::SkeletonData skeleton;
    skeleton.nodes = std::move(nodes);
    skeleton.bone_nodes.reserve(skin.joints_count);
    skeleton.inverse_bind_matrices.reserve(skin.joints_count);

    for (cgltf_size i = 0; i < skin.joints_count; ++i) {
        auto const bone_node = indices.find(skin.joints[i]);
        if (bone_node == indices.end()) {
            OA_LOG_WARN(Log::Model, "{}: bone {} is not part of the node hierarchy, skipping the skin", file_name, i);
            return std::nullopt;
        }
        skeleton.bone_nodes.push_back(bone_node->second);

        auto& inverse_bind = skeleton.inverse_bind_matrices.emplace_back(Math::Mat4f::identity());
        if (skin.inverse_bind_matrices != nullptr) {
            cgltf_accessor_read_float(skin.inverse_bind_matrices, i, inverse_bind.data(), 16);
        }
    }

    OA_LOG_TRACE(Log::Model, "{}: skeleton with {} nodes, {} bones", file_name, skeleton.nodes.size(), skeleton.bone_nodes.size());
    return skeleton;
}

}

auto ModelImporter::import(ImportContext const& context) -> Common::Expected<ModelData>
{
    if (context.registry == nullptr) {
        return OA_ERROR("Cannot import '{}' without an asset registry to resolve its textures against", context.path.string());
    }

    if (!std::filesystem::exists(context.path)) {
        return OA_ERROR("Model file '{}' does not exist", context.path.string());
    }

    auto extension = context.path.extension().string();
    if (extension == ".gltf") {
        return import_gltf(context);
    }

    return OA_ERROR("Unsupported model file extension '{}'", extension);
}

auto ModelImporter::source_hash(ImportContext const& context) -> Common::Expected<u64>
{
    return File::hash_file(context.path);
}

auto ModelImporter::supported_extensions() -> std::vector<std::string>
{
    return { ".gltf" };
}

auto ModelImporter::enumerate_sub_assets(std::filesystem::path const& path) -> Common::Expected<std::vector<SubAssetDescriptor>>
{
    auto const gltf = TRY(glTF::parse(path));

    std::vector<SubAssetDescriptor> descriptors;
    descriptors.reserve(gltf->animations_count);
    for (cgltf_size index = 0; index < gltf->animations_count; ++index) {
        descriptors.push_back({ .name = glTF::clip_name(gltf->animations[index], index), .type = AssetType::Animation });
    }
    return descriptors;
}

auto ModelImporter::import_gltf(ImportContext const& context) -> Common::Expected<ModelData>
{
    auto const& path = context.path;
    auto const* asset_registry = context.registry;
    auto const gltf = TRY(glTF::load(path));
    auto const* data = gltf.get();

    Time::Stopwatch const stopwatch;
    OA_LOG_TRACE(Log::Model, "Importing {}: {} meshes, {} materials", path.filename().string(), data->meshes_count, data->materials_count);

    ModelData model_data;
    model_data.skeleton = import_skeleton(data, path.filename().string());

    for (cgltf_size i = 0; i < data->materials_count; ++i) {
        auto const& gltf_material = data->materials[i];
        auto& material = model_data.materials.emplace_back();
        auto& material_parameters = material.parameters;

        material.name = gltf_material.name ? gltf_material.name : std::format("Material_{}", i);

        auto resolve_texture_id = [&](cgltf_texture_view const& texture_view) -> std::optional<AssetID> {
            if (texture_view.texture == nullptr || texture_view.texture->image == nullptr) {
                return std::nullopt;
            }

            auto const* uri = texture_view.texture->image->uri;
            if (uri == nullptr) {
                OA_LOG_TRACE(Log::Model, "{}: an image of material '{}' has no URI, so it cannot be resolved", path.filename().string(), material.name);
                return std::nullopt;
            }

            auto const texture_path = path.parent_path() / uri;
            auto key = asset_registry->resolve_key(texture_path);
            auto id = asset_registry->key_to_id(key);
            if (!id) {
                OA_LOG_WARN(Log::Model, "{}: texture '{}' is not a registered asset, so materials referencing it fall back to a default", path.filename().string(), uri);
            }
            return id;
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

        OA_LOG_WARN(Log::Model, "{}: a primitive references a material that is not in the file, falling back to material 0", path.filename().string());
        return 0;
    };

    for (cgltf_size i = 0; i < data->meshes_count; ++i) {
        auto const& gltf_mesh = data->meshes[i];

        for (cgltf_size j = 0; j < gltf_mesh.primitives_count; ++j) {
            auto const& primitive = gltf_mesh.primitives[j];
            if (primitive.type != cgltf_primitive_type_triangles) {
                OA_LOG_TRACE(Log::Model, "Dropping primitive {} of mesh {}: only triangle lists are supported", j, i);
                continue;
            }

            auto& sub_mesh = model_data.sub_meshes.emplace_back();
            sub_mesh.material_index = get_material_index(primitive.material);

            cgltf_accessor const* position_accessor = nullptr;
            cgltf_accessor const* normal_accessor = nullptr;
            cgltf_accessor const* tex_coord_accessor = nullptr;
            cgltf_accessor const* tangent_accessor = nullptr;
            cgltf_accessor const* bones_accessor = nullptr;
            cgltf_accessor const* weights_accessor = nullptr;

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
                    bones_accessor = attribute.data;
                    break;
                case cgltf_attribute_type_weights:
                    weights_accessor = attribute.data;
                    break;
                default:
                    break;
                }
            }

            if (position_accessor == nullptr) {
                OA_LOG_TRACE(Log::Model, "Dropping primitive {} of mesh {}: it has no POSITION attribute", j, i);
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

            if (bones_accessor != nullptr && weights_accessor != nullptr && model_data.skeleton.has_value()) {
                auto const bone_limit = static_cast<u32>(model_data.skeleton->bone_nodes.size());
                auto reported_out_of_range = false;

                sub_mesh.skinned_vertices.reserve(vertex_count);
                for (cgltf_size vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
                    Math::Vec4u bone_indices {};
                    cgltf_accessor_read_uint(bones_accessor, vertex_index, &bone_indices.x, Graphics::MAX_BONE_INFLUENCES);

                    Math::Vec4f bone_weights {};
                    cgltf_accessor_read_float(weights_accessor, vertex_index, &bone_weights.x, Graphics::MAX_BONE_INFLUENCES);

                    auto const total = bone_weights.x + bone_weights.y + bone_weights.z + bone_weights.w;
                    if (total > 0.0F) {
                        bone_weights /= total;
                    } else {
                        bone_weights = Math::Vec4f(1.0F, 0.0F, 0.0F, 0.0F);
                    }

                    for (auto* bone : { &bone_indices.x, &bone_indices.y, &bone_indices.z, &bone_indices.w }) {
                        if (*bone >= bone_limit) {
                            if (!reported_out_of_range) {
                                OA_LOG_WARN(Log::Model, "{}: primitive {} of mesh {} references bone {} but the skin only has {}, clamping to 0",
                                    path.filename().string(), j, i, *bone, bone_limit);
                                reported_out_of_range = true;
                            }
                            *bone = 0;
                        }
                    }

                    auto const& vertex = sub_mesh.vertices[vertex_index];
                    sub_mesh.skinned_vertices.push_back({ .position = vertex.position,
                        .tex_coord = vertex.tex_coord,
                        .normal = vertex.normal,
                        .tangent = vertex.tangent,
                        .bone_indices = bone_indices,
                        .bone_weights = bone_weights });
                }

                sub_mesh.vertices.clear();
                sub_mesh.vertices.shrink_to_fit();
            }
        }
    }

    std::erase_if(model_data.sub_meshes, [](auto const& submesh) {
        return submesh.indices.empty();
    });

    OA_LOG_DEBUG(Log::Model, "Imported {}: {} sub meshes, {} materials, {:.1f}ms", path.filename().string(), model_data.sub_meshes.size(), model_data.materials.size(), stopwatch.elapsed_milliseconds());
    return model_data;
}

}
