struct DirectionalLight
{
    vec4 direction;
    vec4 color;
    mat4 space_matrix;
};

layout(set = 0, binding = 1) uniform PerFrameUniform
{
    mat4 u_camera_projection;
    mat4 u_camera_view;
    vec4 u_camera_position;
    DirectionalLight u_directional_light;
};