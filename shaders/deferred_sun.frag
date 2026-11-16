#version 450

#include "utils.glsl"
#include "lighting.glsl"

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 in_uv;

layout(binding = 0) uniform sampler2D in_albedo_roughness;
layout(binding = 1) uniform sampler2D in_normal_metalness;
layout(binding = 2) uniform sampler2D in_depth;
layout(binding = 3) uniform sampler2D in_emissive;

layout(binding = 4) uniform samplerCube in_envmap;
layout(binding = 5) uniform sampler2D brdf_lut;
layout(binding = 6) uniform sampler2DShadow in_shadow_map;

layout(binding = 0) uniform Data {
    FrameData frame;
};

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);
    const float depth = texelFetch(in_depth, coord, 0).r;

    if(depth == 0.0) {
        const vec3 view_dir = normalize(unproject(in_uv, 0.001, frame.camera.inv_view_proj) - frame.camera.position);
        out_color = vec4(texture(in_envmap, view_dir).rgb * frame.ibl_intensity, 1.0);
        return;
    }

    const vec4 albedo_roughness = texelFetch(in_albedo_roughness, coord, 0);
    const vec4 normal_metalness = texelFetch(in_normal_metalness, coord, 0);

    const vec3 base_color = albedo_roughness.rgb;
    const float roughness = albedo_roughness.a;
    const float metallic = normal_metalness.a;
    const vec3 normal = normalize(normal_metalness.rgb * 2.0 - 1.0);

    const vec3 position = unproject(in_uv, depth, frame.camera.inv_view_proj);
    const vec3 view_dir = normalize(frame.camera.position - position);

    const vec3 shadow_position = position + frame.sun_dir * frame.sun_bias;
    const vec4 shadow_coord = frame.shadow_view_proj * vec4(shadow_position, 1.0);
    const vec3 shadow_ndc = shadow_coord.xyz / shadow_coord.w;
    const vec2 shadow_uv = shadow_ndc.xy * 0.5 + 0.5;
    float shadow = 1.0;
    if(all(greaterThanEqual(shadow_uv, vec2(0.0))) &&
       all(lessThanEqual(shadow_uv, vec2(1.0))) && shadow_ndc.z <= 1.0) {
        const float shadow_depth = max(shadow_ndc.z, 0.0);
        const vec2 texel_size = 1.0 / vec2(textureSize(in_shadow_map, 0));
        shadow = 0.0;
        for(int i = -1; i <= 1; ++i) {
            for(int j = -1; j <= 1; ++j) {
                const vec2 uv = shadow_uv + vec2(i, j) * texel_size;
                shadow += texture(in_shadow_map, vec3(uv, shadow_depth));
            }
        }
        shadow /= 9.0;
    }

    vec3 acc = texelFetch(in_emissive, coord, 0).rgb;
    acc += eval_ibl(in_envmap, brdf_lut, normal, view_dir, base_color, metallic, roughness) * frame.ibl_intensity;
    acc += shadow * frame.sun_color * eval_brdf(normal, view_dir, frame.sun_dir, base_color, metallic, roughness);

    out_color = vec4(acc, 1.0);
}
