#version 330 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;

out vec4 light_space_position;
out vec3 normal;

uniform mat4 u_view_projection;
uniform mat4 u_model;
uniform mat4 u_light_space_mat;

void main()
{
    normal = mat3(transpose(inverse(u_model))) * in_normal;
    light_space_position = u_light_space_mat * u_model * vec4(in_position, 1.0);
    gl_Position = u_view_projection * u_model * vec4(in_position, 1.0);
}
