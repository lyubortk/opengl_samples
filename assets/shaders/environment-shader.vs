#version 330 core

layout (location = 0) in vec3 in_position;

out vec3 tex_coords;

uniform mat4 u_view_projection;

void main()
{
    tex_coords = in_position;
    gl_Position = u_view_projection * vec4(in_position, 1.0);
}
