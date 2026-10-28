#version 450

#ifdef ALPHA_TEST
layout(location = 1) in vec2 in_uv;
layout(binding = 0) uniform sampler2D in_texture;

uniform float alpha_cutoff;
#endif

void main() {
#ifdef ALPHA_TEST
    if(texture(in_texture, in_uv).a <= alpha_cutoff) {
        discard;
    }
#endif
}