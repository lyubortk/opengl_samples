#version 330 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;

out vec3 world_position;
out vec3 normal;

uniform mat4 u_view_projection;
uniform mat4 u_model;

void main()
{
    normal = mat3(transpose(inverse(u_model))) * in_normal;
    world_position = vec3(u_model * vec4(in_position, 1.0));
    gl_Position = u_view_projection * u_model * vec4(in_position, 1.0);
}
