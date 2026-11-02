#version 450

#include "utils.glsl"
#include "lighting.glsl"

// fragment shader of the main lighting pass

// #define DEBUG_NORMAL
// #define DEBUG_METAL
// #define DEBUG_ROUGH
// #define DEBUG_ENV

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec3 in_position;
layout(location = 4) in vec3 in_tangent;
layout(location = 5) in vec3 in_bitangent;

layout(binding = 0) uniform sampler2D in_texture;
layout(binding = 1) uniform sampler2D in_normal_texture;
layout(binding = 2) uniform sampler2D in_metal_rough;
layout(binding = 3) uniform sampler2D in_emissive;
layout(binding = 6) uniform sampler2DShadow in_shadow_map;

uniform vec3 base_color_factor;
uniform vec2 metal_rough_factor;
uniform vec3 emissive_factor;
uniform float alpha_cutoff;

layout(binding = 4) uniform samplerCube in_envmap;
layout(binding = 5) uniform sampler2D brdf_lut;

layout(binding = 0) uniform Data {
    FrameData frame;
};

layout(binding = 1) buffer PointLights {
    PointLight point_lights[];
};

void main() {
    const vec3 normal_map = unpack_normal_map(texture(in_normal_texture, in_uv).xy);
    const vec3 normal = normal_map.x * in_tangent +
                        normal_map.y * in_bitangent +
                        normal_map.z * in_normal;

    const vec4 albedo_tex = texture(in_texture, in_uv);
    const vec3 base_color = in_color.rgb * albedo_tex.rgb * base_color_factor;
    const float alpha = albedo_tex.a;

#ifdef ALPHA_TEST
    if(alpha <= alpha_cutoff) {
        discard;
    }
#endif

    // Shadow mapping
    const vec3 shadow_position = in_position + frame.sun_dir * frame.sun_bias;
    const vec4 shadow_coord = frame.shadow_view_proj * vec4(shadow_position, 1.0);
    const vec3 shadow_ndc = shadow_coord.xyz / shadow_coord.w;
    const vec2 shadow_uv = shadow_ndc.xy * 0.5 + 0.5;
    float shadow = 1.0;
    if(all(greaterThanEqual(shadow_uv, vec2(0.0))) &&
       all(lessThanEqual(shadow_uv, vec2(1.0))) && shadow_ndc.z <= 1.0) {
        // Reversed-Z: near is 1, far is 0. Extend shadows beyond the far plane.
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

    const vec4 metal_rough_tex = texture(in_metal_rough, in_uv);
    const float roughness = metal_rough_tex.g * metal_rough_factor.y; // as per glTF spec
    const float metallic = metal_rough_tex.b * metal_rough_factor.x; // as per glTF spec


    const vec3 to_view = (frame.camera.position - in_position);
    const vec3 view_dir = normalize(to_view);

    vec3 acc = texture(in_emissive, in_uv).rgb * emissive_factor;
    acc += eval_ibl(in_envmap, brdf_lut, normal, view_dir, base_color, metallic, roughness) * frame.ibl_intensity;
    {
        acc += shadow * frame.sun_color * eval_brdf(normal, view_dir, frame.sun_dir, base_color, metallic, roughness);

        for(uint i = 0; i != frame.point_light_count; ++i) {
            PointLight light = point_lights[i];
            const vec3 to_light = (light.position - in_position);
            const float dist = length(to_light);
            const vec3 light_vec = to_light / dist;

            const float att = attenuation(dist, light.radius);
            if(att <= 0.0f) {
                continue;
            }

            acc += eval_brdf(normal, view_dir, light_vec, base_color, metallic, roughness) * att * light.color;
        }
    }

    out_color = vec4(acc, alpha);


#ifdef DEBUG_NORMAL
    out_color = vec4(normal * 0.5 + 0.5, 1.0);
#endif

#ifdef DEBUG_METAL
    out_color = vec4(vec3(metallic), 1.0);
#endif

#ifdef DEBUG_ROUGH
    out_color = vec4(vec3(roughness), 1.0);
#endif

#ifdef DEBUG_ENV
    out_color = vec4(texture(in_envmap, normal).rgb, 1.0);
#endif
}

