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

    vec3 reflection_color = texture(u_environment, reflection_vec).rgb;
    vec3 refraction_color = texture(u_environment, refraction_vec).rgb;

    float theta_t = dot(refraction_vec, -normalized_normal);
    float theta_i = dot(-normalized_vision_vec, normalized_normal);

    float z2 = 1.0 / u_refractive_index;
    float rs_root = (z2 * theta_i - theta_t) / (z2 * theta_i + theta_t);
    float rp_root = (z2 * theta_t - theta_i) / (z2 * theta_t + theta_i);

    float reflection_coeff = (rs_root * rs_root + rp_root * rp_root) / 2;
    vec3 final_color = reflection_coeff * reflection_color + (1 - reflection_coeff) * refraction_color;

    o_frag_color = vec4(final_color, 1.0);
}
