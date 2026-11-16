#version 450

#include "utils.glsl"
#include "lighting.glsl"

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D in_albedo_roughness;
layout(binding = 1) uniform sampler2D in_normal_metalness;
layout(binding = 2) uniform sampler2D in_depth;

layout(binding = 0) uniform Data {
    FrameData frame;
};

layout(binding = 1) buffer PointLights {
    PointLight point_lights[];
};

uniform uint light_index;

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);
    const vec2 uv = gl_FragCoord.xy / vec2(textureSize(in_depth, 0));
    const float depth = texelFetch(in_depth, coord, 0).r;

    if(depth == 0.0) {
        discard;
    }

    const vec4 albedo_roughness = texelFetch(in_albedo_roughness, coord, 0);
    const vec4 normal_metalness = texelFetch(in_normal_metalness, coord, 0);

    const vec3 base_color = albedo_roughness.rgb;
    const float roughness = albedo_roughness.a;
    const float metallic = normal_metalness.a;
    const vec3 normal = normalize(normal_metalness.rgb * 2.0 - 1.0);

    const vec3 position = unproject(uv, depth, frame.camera.inv_view_proj);
    const vec3 view_dir = normalize(frame.camera.position - position);

    PointLight light = point_lights[light_index];
    const vec3 to_light = (light.position - position);
    const float dist = length(to_light);

    const float att = attenuation(dist, light.radius);
    if(att <= 0.0f || dist == 0.0) {
        discard;
    }

    const vec3 light_vec = to_light / dist;
    const vec3 acc = eval_brdf(normal, view_dir, light_vec, base_color, metallic, roughness) * att * light.color;

    out_color = vec4(acc, 0.0);
}
