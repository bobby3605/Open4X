#version 460

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
}
ubo;

struct SSBOData {
    mat4 modelMatrix;
};

layout(std140, set = 2, binding = 0) readonly buffer SSBO { SSBOData data[]; }
ssbo;

layout(push_constant) uniform Push {
    mat4 model;
    bool indirect;
}
push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    if (push.indirect) {
        gl_Position = ubo.proj * ubo.view * ssbo.data[gl_BaseInstance].modelMatrix * vec4(inPosition, 1.0);
    } else {
        gl_Position = ubo.proj * ubo.view * push.model * vec4(inPosition, 1.0);
    }
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
