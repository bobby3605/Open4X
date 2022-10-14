#version 460

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
}
ubo;

struct SSBOData {
    mat4 modelMatrix;
};

struct materialData {
    vec4 baseColorFactor;
};

layout(std140, set = 2, binding = 0) readonly buffer SSBO { SSBOData data[]; }
ssbo;

layout(std140, set = 2, binding = 2) readonly buffer Materials { materialData data[]; }
materials;

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
    if (push.indirect) {
        gl_Position = ubo.proj * ubo.view * ssbo.data[gl_InstanceIndex].modelMatrix * vec4(inPosition, 1.0);
        fragColor = materials.data[gl_BaseInstance].baseColorFactor;
    } else {
        gl_Position = ubo.proj * ubo.view * push.model * vec4(inPosition, 1.0);
        fragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    fragTexCoord = inTexCoord;
}
