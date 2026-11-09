#version 450

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D in_albedo_roughness;
layout(binding = 1) uniform sampler2D in_normal_metalness;
layout(binding = 2) uniform sampler2D in_depth;

uniform uint debug_mode;

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);
    const float depth = texelFetch(in_depth, coord, 0).r;

    // The reverse-Z depth buffer is cleared to 0.
    if(depth == 0.0) {
        out_color = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 color = vec3(0.0);
    switch(debug_mode) {
        case 0u: // Depth
            color = vec3(pow(depth, 0.35));
        break;

        case 1u: // Albedo
            color = texelFetch(in_albedo_roughness, coord, 0).rgb;
        break;

        case 2u: // Normals
            color = texelFetch(in_normal_metalness, coord, 0).rgb;
        break;

        case 3u: // Roughness
            color = vec3(texelFetch(in_albedo_roughness, coord, 0).a);
        break;

        case 4u: // Metalness
            color = vec3(texelFetch(in_normal_metalness, coord, 0).a);
        break;
    }

    out_color = vec4(color, 1.0);
}
