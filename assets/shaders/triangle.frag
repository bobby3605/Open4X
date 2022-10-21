#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in flat uint samplerIndex;
layout(location = 3) in flat uint imageIndex;
layout(location = 4) in vec3 normal;
layout(location = 5) in vec3 fragPos;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
}
ubo;

layout(set = 1, binding = 0) uniform sampler samplers[];
layout(set = 1, binding = 1) uniform texture2D images[];

layout(location = 0) out vec4 outColor;

const vec3 lightPos = vec3(0.0, 0.0, -3.0);
const vec4 lightColor = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 ambientColor = vec4(1.0, 1.0, 1.0, 1.0);
const float ambientStrength = 0.5;
const float specularStrength = 1.0;

void main() {

    // Diffuse
    // gltf normal should already be normalized
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec4 diffuse = diff * lightColor;

    // Specular
    vec3 viewPos = vec3(ubo.view[3]);
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), 32);
    vec4 specular = specularStrength * spec * lightColor;

    // Ambient
    vec4 ambient = ambientStrength * ambientColor;

    // Base
    outColor = fragColor * texture(sampler2D(images[nonuniformEXT(imageIndex)], samplers[nonuniformEXT(samplerIndex)]), fragTexCoord);

    outColor = (ambient + diffuse + specular) * outColor;
}
