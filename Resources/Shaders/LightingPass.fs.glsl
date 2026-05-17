#version 460 core

#include <Math.glsl>
#include <Uniforms/PerFrame.glsl>

layout(set = 0, binding = 0) uniform sampler u_texture_sampler;

layout(set = 2, binding = 0) uniform sampler u_shadow_sampler;
layout(set = 2, binding = 1) uniform texture2D u_shadow_map_texture;

layout(location = 0) in vec2 in_tex_coord;
layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform texture2D u_normal_texture;
layout(set = 1, binding = 1) uniform texture2D u_albedo_texture;
layout(set = 1, binding = 2) uniform texture2D u_material_texture;
layout(set = 1, binding = 3) uniform texture2D u_emissive_texture;
layout(set = 1, binding = 4) uniform texture2D u_depth_texture;

vec3 reconstruct_world_pos(vec2 uv, float depth)
{
    vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 view = inverse(u_camera_projection) * clip;
    view /= view.w;
    vec4 world = inverse(u_camera_view) * view;
    return world.xyz;
}

float calculate_shadow(vec3 N, vec3 world_pos)
{
    vec4 light_space_pos = u_directional_light.space_matrix * vec4(world_pos, 1.0);
    light_space_pos /= light_space_pos.w;
    vec2 shadow_uv = light_space_pos.xy * 0.5 + 0.5;

    float closest_depth = texture(sampler2D(u_shadow_map_texture, u_shadow_sampler), shadow_uv).r;
    float current_depth = light_space_pos.z;

    vec3 L = normalize(-u_directional_light.direction.xyz);
    float cos_theta = clamp(dot(N, L), 0.0, 1.0);
    float bias = max(0.0005 * tan(acos(cos_theta)), 0.0001);
    bias = clamp(bias, 0.0, 0.001);
    return current_depth - bias < closest_depth ? 1.0 : 0.0;
}

float distribution_ggx(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float geometry_schlick_ggx(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometry_schlick_ggx(NdotV, roughness);
    float ggx1 = geometry_schlick_ggx(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cos_theta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

vec3 calculate_directional_light_pbr(DirectionalLight light, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness)
{
    vec3 L = normalize(-light.direction.xyz);
    vec3 H = normalize(V + L);

    vec3 radiance = light.color.xyz * light.color.w;

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float NDF = distribution_ggx(N, H, roughness);
    float G   = geometry_smith(N, V, L, roughness);
    vec3  F   = fresnel_schlick(max(dot(H, V), 0.0), F0);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    vec3 specular = (NDF * G * F) / max(4.0 * NdotV * NdotL, 0.0001);

    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / PI;

    return (diffuse + specular) * radiance * NdotL;
}

vec3 tonemap_aces(vec3 x)
{
    float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// -------------------------------------------------------------------------------
void main()
{
    vec3 N = texture(sampler2D(u_normal_texture, u_texture_sampler), in_tex_coord).xyz;
    vec3 albedo = texture(sampler2D(u_albedo_texture, u_texture_sampler), in_tex_coord).rgb;
    vec3 rmao = texture(sampler2D(u_material_texture, u_texture_sampler), in_tex_coord).rgb;
    vec3 emissive = texture(sampler2D(u_emissive_texture, u_texture_sampler), in_tex_coord).rgb;
    float depth = texture(sampler2D(u_depth_texture, u_texture_sampler), in_tex_coord).r;

    float roughness = rmao.r;
    float metallic = rmao.g;
    float occlusion = rmao.b;

    vec3 world_pos = reconstruct_world_pos(in_tex_coord, depth);
    vec3 V = normalize(u_camera_position.xyz - world_pos);
    float shadow = calculate_shadow(N, world_pos);

    vec3 Lo = calculate_directional_light_pbr(u_directional_light, N, V, albedo, metallic, roughness);

    vec3 ambient = vec3(0.08) * albedo * occlusion;
    vec3 color = Lo * shadow + ambient + emissive;
    color = tonemap_aces(color);

    out_color = vec4(color, 1.0);
}