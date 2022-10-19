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
};

struct indicesData {
    uint objectIndex;
    uint materialIndex;
};

layout(std140, set = 2, binding = 0) readonly buffer Objects { objectData data[]; }
objects;

layout(std140, set = 2, binding = 2) readonly buffer Materials { materialData data[]; }
materials;

layout(set = 2, binding = 3) readonly buffer Indices { indicesData data[]; }
indices;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out uint samplerIndex;
layout(location = 3) out uint imageIndex;

void main() {
    indicesData indexData = indices.data[gl_InstanceIndex];

    materialData material = materials.data[indexData.materialIndex];

    gl_Position = ubo.proj * ubo.view * objects.data[indexData.objectIndex].modelMatrix * vec4(inPosition, 1.0);

    fragColor = material.baseColorFactor;
    fragTexCoord = texCoord;

    samplerIndex = material.samplerIndex;
    samplerIndex = material.imageIndex;
}
