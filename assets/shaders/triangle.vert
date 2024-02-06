#version 460

layout(binding = 0) uniform Globals {
    mat4 projView;
    vec3 camPos;
};

struct ObjectData {
    vec3 translation;
    vec4 rotation;
    vec3 scale;
};

struct MaterialData {
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

layout(std140, set = 1, binding = 0) readonly buffer Objects { ObjectData objects[]; };
layout(std140, set = 1, binding = 1) readonly buffer Materials { MaterialData materials[]; };
layout(set = 1, binding = 2) readonly buffer CulledInstanceIndices { uint culledInstanceIndices[]; };
layout(set = 1, binding = 3) readonly buffer CulledMaterialIndices { uint culledMaterialIndices[]; };

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 tangent;

layout(location = 0) out vec4 baseColorFactor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out uint samplerIndex;
layout(location = 3) out uint imageIndex;
layout(location = 4) out uint normalIndex;
layout(location = 5) out uint metallicRoughnessIndex;
layout(location = 6) out uint aoIndex;
layout(location = 7) out vec3 WorldPos;
layout(location = 8) out vec3 Normal;
layout(location = 9) out vec3 camPosOut;
layout(location = 10) out float normalScale;
layout(location = 11) out float metallicFactor;
layout(location = 12) out float roughnessFactor;
layout(location = 13) out float occulsionStrength;

// https://www.geeks3d.com/20141201/how-to-rotate-a-vertex-by-a-quaternion-in-glsl/
vec3 rotate_vertex_position(vec3 position, vec4 rotation)
{
  return position + 2.0 * cross(rotation.xyz, cross(rotation.xyz, position) + rotation.w * position);
}


void main() {
    MaterialData material = materials[culledMaterialIndices[gl_BaseInstance]];

    ObjectData object = objects[culledInstanceIndices[gl_InstanceIndex]];

    // FIXME:
    // only supports uniform scaling
    vec4 vertPos = vec4(rotate_vertex_position(inPosition, object.rotation) * object.scale.x + object.translation, 1.0);

    gl_Position = projView * vertPos;

    baseColorFactor = material.baseColorFactor;

    samplerIndex = material.samplerIndex;
    imageIndex = material.imageIndex;
    normalIndex = material.normalIndex;
    metallicRoughnessIndex = material.metallicRoughnessIndex;
    aoIndex = material.aoIndex;

    normalScale = material.normalScale;
    metallicFactor = material.metallicFactor;
    roughnessFactor = material.roughnessFactor;
    occulsionStrength = material.occlusionStrength;

    fragTexCoord = texCoord;
    WorldPos = vec3(vertPos);
    Normal = normal;
    camPosOut = camPos;
}
