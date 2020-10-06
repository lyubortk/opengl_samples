#version 330 core

in vec3 world_position;
in vec3 normal;

uniform samplerCube u_environment;
uniform vec3 u_camera_pos;
uniform float u_refractive_index;

out vec4 o_frag_color;

void main()
{
    vec3 vision_vec = world_position - u_camera_pos;
    vec3 normalized_vision_vec = normalize(vision_vec);
    vec3 normalized_normal = normalize(normal);
    vec3 reflection_vec = reflect(normalized_vision_vec, normalized_normal);
    vec3 refraction_vec = refract(normalized_vision_vec, normalized_normal, 1.0 / u_refractive_index);
    o_frag_color = vec4(texture(u_environment, refraction_vec).rgb, 1.0);
}
