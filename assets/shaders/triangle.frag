#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in flat uint samplerIndex;
layout(location = 3) in flat uint imageIndex;
layout(location = 4) in flat uint normalIndex;
layout(location = 5) in vec3 tangentLightPos;
layout(location = 6) in vec3 tangentViewPos;
layout(location = 7) in vec3 tangentFragPos;
layout(location = 8) in vec3 fragNormal;

layout(set = 1, binding = 0) uniform sampler samplers[];
layout(set = 1, binding = 1) uniform texture2D images[];
layout(set = 1, binding = 2) uniform texture2D normals[];

layout(location = 0) out vec4 outColor;

const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const vec3 ambientColor = vec3(1.0, 1.0, 1.0);
const float ambientStrength = 0.1;
const vec3 specularStrength = vec3(0.2);

void main() {

    // Diffuse
    vec3 normal;
    if (normalIndex == 0) {
        normal = fragNormal;
    } else {
        normal = texture(sampler2D(normals[nonuniformEXT(normalIndex)], samplers[nonuniformEXT(samplerIndex)]), fragTexCoord).rgb;
        normal = normalize(normal * 2.0 - 1.0);
    }
    // convert +y normals to vulkan -y
    normal = vec3(normal.r, 1.0 - normal.g, normal.b);
    vec3 lightDir = normalize(tangentLightPos - tangentFragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular
    vec3 viewDir = normalize(tangentViewPos - tangentFragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), 32);
    vec3 specular = specularStrength * spec;

    // Ambient
    vec3 ambient = ambientStrength * ambientColor;

    // Base color
    outColor = fragColor * texture(sampler2D(images[nonuniformEXT(imageIndex)], samplers[nonuniformEXT(samplerIndex)]), fragTexCoord);

    outColor = vec4(diffuse + ambient + specular, 1.0) * outColor;
}
