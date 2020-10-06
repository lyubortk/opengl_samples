#version 330 core

layout (location = 0) in vec3 in_position;

out vec3 tex_coords;

uniform mat4 u_mvp;

void main()
{
    tex_coords = in_position;
    gl_Position = u_mvp * vec4(in_position, 1.0);
}
