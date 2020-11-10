#version 330 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texture_coords;
layout (location = 3) in float in_height;

out vec3 world_position;
out vec3 normal;
out vec2 texture_coords;
out vec4 light_space_position;
out float height;

uniform mat4 u_view_projection;
uniform mat4 u_light_space_mat;
uniform mat4 u_model;

void main()
{
    normal = mat3(transpose(inverse(u_model))) * in_normal;
    world_position = vec3(u_model * vec4(in_position, 1.0));
    light_space_position = u_light_space_mat * vec4(world_position, 1.0);
    texture_coords = in_texture_coords;
    height = in_height;
    gl_Position = u_view_projection * u_model * vec4(in_position, 1.0);
}
