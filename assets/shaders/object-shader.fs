#version 330 core

in vec3 normal;
in vec4 light_space_position;

out vec4 o_frag_color;

uniform sampler2D u_shadow_map;
uniform vec3 u_sun_dir;
uniform vec3 u_sun_color;
uniform vec3 u_object_color;
uniform vec3 u_ambient_color;
uniform float u_ambient_strength;
uniform float u_shadow_map_bias;

void main()
{
    vec3 light_space_tex_coords = light_space_position.xyz /  light_space_position.w * 0.5 + 0.5;
    float closest_depth = texture(u_shadow_map, light_space_tex_coords.xy).r;
    float current_depth = light_space_tex_coords.z;
    float shadow_strength = current_depth - u_shadow_map_bias > closest_depth ? 1.0 : 0.0;

    float sun_strength = max(dot(normal, u_sun_dir), 0.0) * (1 - shadow_strength);

    o_frag_color = vec4((sun_strength * u_sun_color + u_ambient_strength * u_ambient_color) * u_object_color,  1.0);
}
