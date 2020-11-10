#version 330 core

in vec3 world_position;
in vec3 normal;
in vec2 texture_coords;
in float height;
in vec4 light_space_position;

uniform samplerCube u_environment;
uniform vec3 u_camera_pos;
uniform sampler2D u_summer;
uniform sampler2D u_winter;
uniform sampler2D u_shadow_map;
uniform float u_summer_threshold;
uniform vec3 u_headlight_pos;
uniform vec3 u_headlight_dir;
uniform float u_headlight_outer_cutoff;
uniform float u_headlight_inner_cutoff;
uniform vec3 u_headlight_color;
uniform vec3 u_sun_dir;
uniform vec3 u_sun_color;
uniform vec3 u_ambient_color;
uniform float u_ambient_strength;
uniform float u_shadow_map_bias;

out vec4 o_frag_color;

void main()
{
    vec3 winter_color = texture(u_winter, texture_coords).rgb;
    vec3 summer_color = texture(u_summer, texture_coords).rgb;
    float summer_coeff_raw = (u_summer_threshold + 0.1 - height) / 0.2;
    float summer_coeff = clamp(summer_coeff_raw, 0.0, 1.0);
    vec3 object_color = summer_coeff * summer_color + (1 - summer_coeff) * winter_color;

    vec3 light_space_tex_coords = light_space_position.xyz /  light_space_position.w * 0.5 + 0.5;
    float closest_depth = texture(u_shadow_map, light_space_tex_coords.xy).r;
    float current_depth = light_space_tex_coords.z;
    float shadow_strength = current_depth - u_shadow_map_bias > closest_depth  ? 1.0 : 0.0;

    float sun_strength = max(dot(normal, u_sun_dir), 0.0) * (1 - shadow_strength);

    vec3 headlight_color = vec3(1, 1, 1);
    vec3 light_to_fragment_dir = normalize(world_position - u_headlight_pos);
    float headlight_distance = length(world_position - u_headlight_pos);
    float light_to_fragment_cos = dot(light_to_fragment_dir, u_headlight_dir);
    float headlight_strength = 1.0 / (1 + 0.65 * headlight_distance + 0.24 * headlight_distance * headlight_distance) *
      clamp((light_to_fragment_cos - u_headlight_outer_cutoff) / (u_headlight_inner_cutoff - u_headlight_outer_cutoff), 0.0, 1.0);

    o_frag_color = vec4(
      (sun_strength * u_sun_color +
       u_ambient_strength * u_ambient_color +
       headlight_strength * headlight_color) * object_color,
      1.0);
}
