const uint MAX_BONES = 128;

layout(set = 2, binding = 0) uniform PerSkinUniform {
    mat4 u_bones[MAX_BONES];
};

mat4 skin_matrix(uvec4 bone_indices, vec4 bone_weights)
{
    return bone_weights.x * u_bones[bone_indices.x]
         + bone_weights.y * u_bones[bone_indices.y]
         + bone_weights.z * u_bones[bone_indices.z]
         + bone_weights.w * u_bones[bone_indices.w];
}
