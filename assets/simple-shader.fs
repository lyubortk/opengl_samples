#version 330 core

uniform sampler1D u_texture;
uniform vec2 u_c;
uniform int u_iter;
uniform int u_display_w;
uniform int u_display_h;
uniform int u_display_min;
uniform vec2 u_shift;
uniform float u_zoom;

void main() {
    vec2 z = (gl_FragCoord.xy * 2.0 - vec2(u_display_w, u_display_h)) / u_display_min;
    z = z * u_zoom + u_shift;

    int i;
    for(i = 0; i < u_iter; i++) {
        float x = (z.x * z.x - z.y * z.y) + u_c.x;
        float y = (z.y * z.x + z.x * z.y) + u_c.y;

        if (x * x + y * y > 4.0) {
            break;
        }
        z.x = x;
        z.y = y;
    }

    gl_FragColor = texture(u_texture, i * 1.0 / u_iter);
}
