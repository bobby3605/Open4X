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
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out uint samplerIndex;
layout(location = 3) out uint imageIndex;
layout(location = 4) out vec3 fragNormal;
layout(location = 5) out vec3 fragPos;
/*
layout(location = 4) out vec3 tangentLightPos;
layout(location = 4) out vec3 tangentViewPos;
layout(location = 4) out vec3 tangentFragPos;
*/

void main() {
    indicesData indexData = indices.data[gl_InstanceIndex];

    materialData material = materials.data[indexData.materialIndex];

    mat4 modelMatrix = objects.data[indexData.objectIndex].modelMatrix;

    vec4 vertPos = modelMatrix * vec4(inPosition, 1.0);

    gl_Position = ubo.proj * ubo.view * vertPos;

    fragColor = material.baseColorFactor;
    fragTexCoord = texCoord;

    samplerIndex = material.samplerIndex;
    imageIndex = material.imageIndex;
    /*
        // Normal
        // https://learnopengl.com/Advanced-Lighting/Normal-Mapping
        vec3 T = normalize(vec3(modelMatrix * vec4(tangent, 0.0)));
        vec3 N = normalize(vec3(modelMatrix * vec4(normal, 0.0)));
        // re-orthogonalize T with respect to N
        T = normalize(T - dot(T, N) * N);
        // then retrieve perpendicular vector B with the cross product of T and N
        vec3 B = cross(N, T);
        mat3 TBN = mat3(T, B, N);
        */

    // FIXME:
    // do this on the cpu
    fragNormal = mat3(transpose(inverse(modelMatrix))) * normal;
    fragPos = vec3(vertPos);
}
