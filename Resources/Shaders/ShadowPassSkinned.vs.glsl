#version 460 core

#include <Uniforms/PerObject.glsl>
#include <Uniforms/PerFrame.glsl>
#include <Uniforms/PerSkin.glsl>

layout(location = 0) in vec3 in_position;
layout(location = 4) in uvec4 in_bone_indices;
layout(location = 5) in vec4 in_bone_weights;

void main()
{
    mat4 model = u_model * skin_matrix(in_bone_indices, in_bone_weights);
    gl_Position = u_directional_light.space_matrix * model * vec4(in_position, 1.0);
}
