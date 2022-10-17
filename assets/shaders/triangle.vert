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
    sampler2D texSampler;
};

struct indicesData {
    uint objectIndex;
    uint materialIndex;
    uint texcoordIndex;
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
layout(location = 2) out sampler2D fragTexSampler;

void main() {
    indicesData indexData = indices.data[gl_InstanceIndex];

    gl_Position = ubo.proj * ubo.view * objects.data[indexData.objectIndex].modelMatrix * vec4(inPosition, 1.0);

    fragColor = materials.data[indexData.materialIndex].baseColorFactor;
    fragTexCoord = texcoords.data[indexData.texcoordIndex];
    fragTexSampler = materials.data[indexData.materialIndex].texSampler;
}
