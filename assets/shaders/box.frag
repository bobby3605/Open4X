#version 460

layout(location = 0) in flat vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = inColor;
}
