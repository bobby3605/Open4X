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
    uint texCoordSelector;
    uint samplerIndex;
};

struct indicesData {
    uint objectIndex;
    uint materialIndex;
    uint texCoordIndex;
    uint verticesCount;
};

layout(std140, set = 2, binding = 0) readonly buffer Objects { objectData data[]; }
objects;

layout(std140, set = 2, binding = 2) readonly buffer Materials { materialData data[]; }
materials;

layout(set = 2, binding = 3) readonly buffer Indices { indicesData data[]; }
indices;

layout(set = 2, binding = 4) readonly buffer Texcoords { vec2 data[]; }
texcoords;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out uint samplerIndex;

void main() {
    indicesData indexData = indices.data[gl_InstanceIndex];

    materialData material = materials.data[indexData.materialIndex];

    gl_Position = ubo.proj * ubo.view * objects.data[indexData.objectIndex].modelMatrix * vec4(inPosition, 1.0);

    fragColor = material.baseColorFactor;
    // 0 verticesCount indicates a default sampler
    if (indexData.verticesCount != 0) {
        // FIXME:
        // this can't be right
        // I should be getting the tex coords for the vertex
        fragTexCoord = texcoords.data[indexData.texCoordIndex + (material.texCoordSelector * indexData.verticesCount)];
    } else {
        fragTexCoord = texcoords.data[0];
    }
    samplerIndex = material.samplerIndex;
}
