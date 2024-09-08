#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in flat vec4 base_color_factor;
layout(location = 1) in vec2 frag_tex_coord;
layout(location = 2) in flat uint sampler_index;
layout(location = 3) in flat uint base_texture_index;
layout(location = 4) in flat uint normal_index;
layout(location = 5) in flat uint metallic_roughness_index;
layout(location = 6) in flat uint ao_index;
layout(location = 7) in vec3 world_pos;
layout(location = 8) in vec3 normal;
layout(location = 9) in flat vec3 cam_pos;
layout(location = 10) in flat float normal_scale;
layout(location = 11) in flat float metallic_factor;
layout(location = 12) in flat float roughness_factor;
layout(location = 13) in flat float occlusion_strength;
layout(set = 2, binding = 0) uniform sampler samplers[];
layout(set = 2, binding = 1) uniform texture2D textures[];

layout(location = 0) out vec4 out_color;

const float PI = 3.14159265359;

// https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/1.2.lighting_textured/1.2.pbr.fs
vec3 get_normal_from_map() {
    // fix for default normal map
    if (normal_index == 0) {
        return normal;
    } else {
        vec3 tangent_normal =
            (normal_scale *
             texture(sampler2D(textures[nonuniformEXT(normal_index)], samplers[nonuniformEXT(sampler_index)]), frag_tex_coord)
                 .rgb) *
                2.0 -
            1.0;

        vec3 Q1 = dFdx(world_pos);
        vec3 Q2 = dFdy(world_pos);
        vec2 st1 = dFdx(frag_tex_coord);
        vec2 st2 = dFdy(frag_tex_coord);

        vec3 N = normalize(normal);
        vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
        vec3 B = -normalize(cross(N, T));
        mat3 TBN = mat3(T, B, N);

        return normalize(TBN * tangent_normal);
    }
}

// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0) { return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0); }
// ----------------------------------------------------------------------------

void main(){
    const uint light_count = 3;
    vec3 light_positions[light_count];
    light_positions[0] = vec3(0.0, 1.0, -2.0);
    light_positions[1] = vec3(0.0, 1.0, 5.0);
    light_positions[2] = vec3(-2.0, -3.0, -2.0);
    vec3 light_colors[light_count];
    light_colors[0] = 10 * vec3(1.0, 1.0, 1.0);
    light_colors[1] = vec3(1.0, 1.0, 1.0);
    light_colors[2] = 10 * vec3(1.0, 1.0, 1.0);
    // PBR
    // https://learnopengl.com/PBR/Lighting
    vec3 albedo =
        pow(vec3(base_color_factor) *
                texture(sampler2D(textures[nonuniformEXT(base_texture_index)], samplers[nonuniformEXT(sampler_index)]), frag_tex_coord)
                    .rgb,
            vec3(2.2));

    vec4 metallic_roughness_texture = texture(
        sampler2D(textures[nonuniformEXT(metallic_roughness_index)], samplers[nonuniformEXT(sampler_index)]),
        frag_tex_coord);

    float metallic = metallic_factor * metallic_roughness_texture.b;
    float roughness = roughness_factor * metallic_roughness_texture.g;

    float ao = occlusion_strength *
               texture(sampler2D(textures[nonuniformEXT(ao_index)], samplers[nonuniformEXT(sampler_index)]), frag_tex_coord).r;

    vec3 N = get_normal_from_map();
    vec3 V = normalize(cam_pos - world_pos);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < light_count; ++i) {
        // calculate per-light radiance
        vec3 L = normalize(light_positions[i] - world_pos);
        vec3 H = normalize(V + L);
        float distance = length(light_positions[i] - world_pos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = light_colors[i] * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;

        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance *
              NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    // ambient lighting (note that the next IBL tutorial will replace
    // this ambient lighting with environment lighting).
    // NOTE:
    // IBL should change ambient and make the non-textured materials brighter
    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0 / 2.2));

    out_color = vec4(color, 1.0);
}
