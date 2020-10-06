#version 330 core

in vec3 tex_coords;

out vec4 o_frag_color;

uniform samplerCube environment;

void main()
{
    o_frag_color = texture(environment, tex_coords);
}
