#version 450

#ifdef ALPHA_TEST
layout(location = 1) in vec2 in_uv;
layout(binding = 0) uniform sampler2D in_texture;

uniform float alpha_cutoff;
uniform vec4 base_color_factor;
#endif

void main() {
#ifdef ALPHA_TEST
    if(texture(in_texture, in_uv).a * base_color_factor.a <= alpha_cutoff) {
        discard;
    }
#endif
}
