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

layout(push_constant) uniform Push {
    mat4 model;
    bool indirect;
}
push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    if (push.indirect == true) {
        gl_Position = ubo.proj * ubo.view * objects.data[indices.data[gl_InstanceIndex].objectIndex].modelMatrix * vec4(inPosition, 1.0);
        fragColor = materials.data[indices.data[gl_InstanceIndex].materialIndex].baseColorFactor;
    } else {
        gl_Position = ubo.proj * ubo.view * push.model * vec4(inPosition, 1.0);
        fragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    fragTexCoord = inTexCoord;
}
