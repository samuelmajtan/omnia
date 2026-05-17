#version 450 core

#include <Uniforms/PerMaterial.glsl>

layout(set = 0, binding = 0) uniform sampler u_texture_sampler;

layout(location = 0) in vec3 in_world_pos;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec4 in_tangent;

layout(location = 0) out vec4 out_normal_pixel;
layout(location = 1) out vec4 out_albedo_pixel;
layout(location = 2) out vec4 out_material_pixel;
layout(location = 3) out vec4 out_emissive_pixel;

void main()
{
    vec4 albedo_sample = texture(sampler2D(u_albedo_texture, u_texture_sampler), in_tex_coord);
    if (albedo_sample.a < 0.1)
        discard;
    out_albedo_pixel = vec4(albedo_sample.rgb, 1.0);

    vec3 N = normalize(in_normal);
    vec3 T = normalize(in_tangent.xyz);
    T = normalize(T - N * dot(N, T));
    vec3 B = cross(N, T) * in_tangent.w;
    mat3 TBN = mat3(T, B, N);

    vec3 normal_sample = texture(sampler2D(u_normal_texture, u_texture_sampler), in_tex_coord).rgb;
    normal_sample = normal_sample * 2.0 - 1.0;
    normal_sample.xy *= u_material_parameters.normal_scale;
    normal_sample.z = sqrt(max(0.0, 1.0 - dot(normal_sample.xy, normal_sample.xy)));
    vec3 normal = normalize(TBN * normal_sample);
    out_normal_pixel = vec4(normal, 1.0);

    float ao_sample = texture(sampler2D(u_occlusion_texture, u_texture_sampler), in_tex_coord).r;
    float occlusion = 1.0 - (1.0 - ao_sample) * u_material_parameters.occlusion_strength;

    vec4 mr_sample = texture(sampler2D(u_metallic_roughness_texture, u_texture_sampler), in_tex_coord);
    float metallic = mr_sample.b * u_material_parameters.metallic_factor;
    float roughness = mr_sample.g * u_material_parameters.roughness_factor;
    out_material_pixel = vec4(roughness, metallic, occlusion, 0.0);

    vec3 emissive_sample = texture(sampler2D(u_emissive_texture, u_texture_sampler), in_tex_coord).rgb;
    vec3 emissive = emissive_sample * u_material_parameters.emissive_factor.xyz;
    out_emissive_pixel = vec4(emissive, 0.0);
}
