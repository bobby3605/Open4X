#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in flat uint samplerIndex;

layout(set = 1, binding = 0) uniform sampler2D samplers2D[];

layout(location = 0) out vec4 outColor;

void main() { outColor = fragColor * texture(samplers2D[nonuniformEXT(samplerIndex)], fragTexCoord); }
