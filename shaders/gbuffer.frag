#version 450

#include "utils.glsl"

layout(location = 0) out vec4 out_albedo_roughness;
layout(location = 1) out vec4 out_normal_metalness;

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_color;
layout(location = 4) in vec3 in_tangent;
layout(location = 5) in vec3 in_bitangent;

layout(binding = 0) uniform sampler2D in_texture;
layout(binding = 1) uniform sampler2D in_normal_texture;
layout(binding = 2) uniform sampler2D in_metal_rough;

uniform vec3 base_color_factor;
uniform vec2 metal_rough_factor;
uniform float alpha_cutoff;

void main() {
    const vec3 normal_map = unpack_normal_map(texture(in_normal_texture, in_uv).xy);
    const vec3 normal = normalize(normal_map.x * in_tangent +
                                  normal_map.y * in_bitangent +
                                  normal_map.z * in_normal);

    const vec4 albedo_tex = texture(in_texture, in_uv);
    const vec3 base_color = in_color.rgb * albedo_tex.rgb * base_color_factor;
    const float alpha = albedo_tex.a;

#ifdef ALPHA_TEST
    if(alpha <= alpha_cutoff) {
        discard;
    }
#endif

    const vec4 metal_rough_tex = texture(in_metal_rough, in_uv);
    const float roughness = metal_rough_tex.g * metal_rough_factor.y; // as per glTF spec
    const float metallic = metal_rough_tex.b * metal_rough_factor.x; // as per glTF spec

    out_albedo_roughness = vec4(base_color, roughness);
    out_normal_metalness = vec4(normal * 0.5 + 0.5, metallic);
}
