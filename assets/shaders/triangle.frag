#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in flat uint samplerIndex;
layout(location = 3) in flat uint imageIndex;

layout(set = 1, binding = 0) uniform sampler samplers[];
layout(set = 1, binding = 1) uniform texture2D images[];

layout(location = 0) out vec4 outColor;

void main() {
    outColor = fragColor * texture(sampler2D(images[nonuniformEXT(imageIndex)], samplers[nonuniformEXT(samplerIndex)]), fragTexCoord);
}
