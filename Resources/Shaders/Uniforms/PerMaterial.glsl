struct MaterialParameters
{
    vec4 base_color;
    vec4 emissive_factor;
    float metallic_factor;
    float roughness_factor;
    float normal_scale;
    float occlusion_strength;
};

layout(set = 1, binding = 0) uniform PerMaterialUniform {
    MaterialParameters u_material_parameters;
};
layout(set = 1, binding = 1) uniform texture2D u_albedo_texture;
layout(set = 1, binding = 2) uniform texture2D u_metallic_roughness_texture;
layout(set = 1, binding = 3) uniform texture2D u_normal_texture;
layout(set = 1, binding = 4) uniform texture2D u_occlusion_texture;
layout(set = 1, binding = 5) uniform texture2D u_emissive_texture;