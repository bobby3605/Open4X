#version 460

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
}
ubo;

struct objectData {
    mat4 modelMatrix;
};

struct materialData {
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

struct culledInstanceIndicesData {
    uint objectIndex;
    uint materialIndex;
};

layout(std140, set = 2, binding = 0) readonly buffer Objects { objectData data[]; }
objects;

layout(std140, set = 2, binding = 1) readonly buffer Materials { materialData data[]; }
materials;

layout(set = 2, binding = 2) readonly buffer CulledInstanceIndices { culledInstanceIndicesData data[]; }
culledInstanceIndices;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 tangent;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out uint samplerIndex;
layout(location = 3) out uint imageIndex;
layout(location = 4) out uint normalIndex;
layout(location = 5) out uint metallicRoughnessIndex;
layout(location = 6) out uint aoIndex;
layout(location = 7) out vec3 WorldPos;
layout(location = 8) out vec3 Normal;
layout(location = 9) out vec3 camPos;
layout(location = 10) out float normalScale;
layout(location = 11) out float metallicFactor;
layout(location = 12) out float roughnessFactor;
layout(location = 13) out float occulsionStrength;

void main() {
    culledInstanceIndicesData culledInstanceIndexData = culledInstanceIndices.data[gl_InstanceIndex];

    objectData object = objects.data[culledInstanceIndexData.objectIndex];
    materialData material = materials.data[culledInstanceIndexData.materialIndex];

    mat4 modelMatrix = object.modelMatrix;

    vec4 vertPos = modelMatrix * vec4(inPosition, 1.0);

    gl_Position = ubo.proj * ubo.view * vertPos;

    fragColor = material.baseColorFactor;
    fragTexCoord = texCoord;

    samplerIndex = material.samplerIndex;
    imageIndex = material.imageIndex;
    normalIndex = material.normalIndex;
    metallicRoughnessIndex = material.metallicRoughnessIndex;
    aoIndex = material.aoIndex;

    normalScale = material.normalScale;
    metallicFactor = material.metallicFactor;
    roughnessFactor = material.roughnessFactor;
    occulsionStrength = material.occlusionStrength;

    WorldPos = vec3(vertPos);
    Normal = normal;
    camPos = vec3(ubo.view[3]);
}
