#version 330 core

in vec3 world_position;
in vec3 normal;

out vec4 o_frag_color;

void main()
{
    float ambient_strength = 0.5;
    vec3 ambient_color = vec3(1, 1, 1);

    vec3 sun_position = normalize(vec3(-1, 0.7, -1));
    float sun_strength = max(dot(normal, sun_position), 0.0);
    vec3 sun_color = vec3(0.9, 0.9, 0.9);

    vec3 object_color = vec3(0.19, 0.3, 0.3);
    o_frag_color = vec4((sun_strength * sun_color + ambient_strength * ambient_color) * object_color,  1.0);
}
