#version 450 core

#include <Uniforms/PerFrame.glsl>
#include <Uniforms/PerObject.glsl>
#include <Uniforms/PerSkin.glsl>

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec4 in_tangent;
layout(location = 4) in uvec4 in_bone_indices;
layout(location = 5) in vec4 in_bone_weights;

layout(location = 0) out vec3 out_world_position;
layout(location = 1) out vec2 out_tex_coord;
layout(location = 2) out vec3 out_normal;
layout(location = 3) out vec4 out_tangent;

void main()
{
    mat4 skin = skin_matrix(in_bone_indices, in_bone_weights);
    mat4 model = u_model * skin;

    out_tex_coord = in_tex_coord;
    out_world_position = vec3(model * vec4(in_position, 1.0));
    gl_Position = u_camera_projection * u_camera_view * vec4(out_world_position, 1.0);

    mat3 normal_matrix = transpose(inverse(mat3(model)));
    out_normal = normal_matrix * in_normal;
    out_tangent = vec4(normal_matrix * in_tangent.xyz, in_tangent.w);
}
