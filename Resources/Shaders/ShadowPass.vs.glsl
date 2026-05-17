#version 460 core

#include <Uniforms/PerObject.glsl>
#include <Uniforms/PerFrame.glsl>

layout(location = 0) in vec3 in_position;

void main()
{
    gl_Position = u_directional_light.space_matrix * u_model * vec4(in_position, 1.0);
}
