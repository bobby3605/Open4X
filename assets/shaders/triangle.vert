#version 460

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
}
ubo;

struct ObjectData {
    mat4 modelMatrix;
};

struct MaterialData {
    vec4 baseColorFactor;
    uint samplerIndex;
    uint imageIndex;
    uint normalIndex;
    float normalScale;
    uint metallicRoughnessIndex;
    float metallicFactor;
    float roughnessFactor;
    uint aoIndex;
    float occlusionStrength;
};

layout(std140, set = 2, binding = 0) readonly buffer Objects { ObjectData objects[]; };

layout(std140, set = 2, binding = 1) readonly buffer Materials { MaterialData materials[]; };

layout(set = 2, binding = 2) readonly buffer CulledInstanceIndices { uint culledInstanceIndices[]; };

layout(set = 2, binding = 3) readonly buffer CulledMaterialIndices { uint culledMaterialIndices[]; };

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 tangent;

layout(location = 0) out MaterialData material;
layout(location = 10) out vec2 fragTexCoord;
layout(location = 11) out vec3 WorldPos;
layout(location = 12) out vec3 Normal;
layout(location = 13) out vec3 camPos;

void main() {
    material = materials[culledMaterialIndices[gl_BaseInstance]];

    ObjectData object = objects[culledInstanceIndices[gl_InstanceIndex]];

    mat4 modelMatrix = object.modelMatrix;

    vec4 vertPos = modelMatrix * vec4(inPosition, 1.0);

    gl_Position = ubo.proj * ubo.view * vertPos;

    fragTexCoord = texCoord;
    WorldPos = vec3(vertPos);
    Normal = normal;
    camPos = vec3(ubo.view[3]);
}
