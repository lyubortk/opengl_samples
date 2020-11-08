#version 330 core

in vec3 world_position;
in vec3 normal;
in vec2 texture_coords;
in float height;

uniform samplerCube u_environment;
uniform vec3 u_camera_pos;
uniform sampler2D u_summer;
uniform sampler2D u_winter;

uniform float u_summer_threshold;

out vec4 o_frag_color;

void main()
{
    vec3 winter_color = texture(u_winter, texture_coords).rgb;
    vec3 summer_color = texture(u_summer, texture_coords).rgb;
    float summer_coeff_raw = (u_summer_threshold + 0.1 - height) / 0.2;
    float summer_coeff = max(min(summer_coeff_raw, 1.0), 0.0);
    vec3 object_color = summer_coeff * summer_color + (1 - summer_coeff) * winter_color;

    float ambient_strength = 0.5;
    vec3 ambient_color = vec3(1, 1, 1);

    vec3 sun_position = normalize(vec3(-1, 0.7, -1));
    float sun_strength = max(dot(normal, sun_position), 0.0);
    vec3 sun_color = vec3(0.9, 0.9, 0.9);

    o_frag_color = vec4((sun_strength * sun_color + ambient_strength * ambient_color) * object_color,  1.0);
}
