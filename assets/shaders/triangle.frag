// All learnopengl.com code is licensed under CC BY-NC 4.0
// Joey de Vries
// https://twitter.com/JoeyDeVriez
// https://creativecommons.org/licenses/by-nc/4.0/legalcode
// modified for GLTF 2.0

#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in flat vec4 baseColorFactor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in flat uint samplerIndex;
layout(location = 3) in flat uint imageIndex;
layout(location = 4) in flat uint normalIndex;
layout(location = 5) in flat uint metallicRoughnessIndex;
layout(location = 6) in flat uint aoIndex;
layout(location = 7) in vec3 WorldPos;
layout(location = 8) in vec3 Normal;
layout(location = 9) in flat vec3 camPos;
layout(location = 10) in flat float normalScale;
layout(location = 11) in flat float metallicFactor;
layout(location = 12) in flat float roughnessFactor;
layout(location = 13) in flat float occlusionStrength;
layout(set = 1, binding = 0) uniform sampler samplers[];
layout(set = 1, binding = 1) uniform texture2D images[];
layout(set = 1, binding = 2) uniform texture2D normals[];
layout(set = 1, binding = 3) uniform texture2D metallicRoughnesses[];
layout(set = 1, binding = 4) uniform texture2D aos[];

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

// https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/1.2.lighting_textured/1.2.pbr.fs
vec3 getNormalFromMap() {
    // fix for default normal map
    if (normalIndex == 0) {
        return Normal;
    } else {
        vec3 tangentNormal =
            (normalScale *
             texture(sampler2D(normals[nonuniformEXT(normalIndex)], samplers[nonuniformEXT(samplerIndex)]), fragTexCoord)
                 .rgb) *
                2.0 -
            1.0;

        vec3 Q1 = dFdx(WorldPos);
        vec3 Q2 = dFdy(WorldPos);
        vec2 st1 = dFdx(fragTexCoord);
        vec2 st2 = dFdy(fragTexCoord);

        vec3 N = normalize(Normal);
        vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
        vec3 B = -normalize(cross(N, T));
        mat3 TBN = mat3(T, B, N);

        return normalize(TBN * tangentNormal);
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

void main() {
    const uint lightCount = 2;
    vec3 lightPositions[lightCount];
    lightPositions[0] = vec3(0.0, 1.0, -2.0);
    lightPositions[1] = vec3(0.0, 1.0, 5.0);
    vec3 lightColors[lightCount];
    lightColors[0] = 10 * vec3(1.0, 1.0, 1.0);
    lightColors[1] = vec3(1.0, 1.0, 1.0);
    // PBR
    // https://learnopengl.com/PBR/Lighting
    vec3 albedo =
        pow(vec3(baseColorFactor) *
                texture(sampler2D(images[nonuniformEXT(imageIndex)], samplers[nonuniformEXT(samplerIndex)]), fragTexCoord)
                    .rgb,
            vec3(2.2));

    vec4 metallicRoughnessTexture = texture(
        sampler2D(metallicRoughnesses[nonuniformEXT(metallicRoughnessIndex)], samplers[nonuniformEXT(samplerIndex)]),
        fragTexCoord);

    float metallic = metallicFactor * metallicRoughnessTexture.b;
    float roughness = roughnessFactor * metallicRoughnessTexture.g;

    float ao = occlusionStrength *
               texture(sampler2D(aos[nonuniformEXT(aoIndex)], samplers[nonuniformEXT(samplerIndex)]), fragTexCoord).r;

    vec3 N = getNormalFromMap();
    vec3 V = normalize(camPos - WorldPos);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < lightCount; ++i) {
        // calculate per-light radiance
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;

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

    outColor = vec4(color, 1.0);
}
