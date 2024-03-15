#version 460

layout(binding = 0) uniform Globals {
    mat4 projView;
    vec3 camPos;
};

struct OBB {
    vec3 center;
    vec3 half_extents;
    vec3 directionU;
    vec3 directionV;
    vec3 directionW;
};

struct ObjectCullData {
    OBB obb;
    uint firstInstanceID;
    uint instanceCount;
};

/*
const vec3 boxVertices[8] = vec3[8](vec3(0.5, 0.5, 0.5),
                                  vec3(0.5, 0.5, -0.5),
                                  vec3(0.5, -0.5, 0.5),
                                  vec3(0.5, -0.5, -0.5),
                                  vec3(-0.5, 0.5, 0.5),
                                  vec3(-0.5, 0.5, -0.5),
                                  vec3(-0.5, -0.5, 0.5),
                                  vec3(-0.5, -0.5, -0.5));

const vec2 boxLines[12] = vec2[12](vec2(0, 1),
                               vec2(0, 2),
                               vec2(0, 4),
                               vec2(1, 5),
                               vec2(1, 3),
                               vec2(2, 3),
                               vec2(3, 7),
                               vec2(2, 6),
                               vec2(4, 5),
                               vec2(4, 6),
                               vec2(5, 7),
                               vec2(7, 6));
                               */

layout(set = 0, binding = 0) readonly buffer ObjectCullingData { ObjectCullData obbs[]; };

layout(location = 0) in vec3 inPosition;

void main() {
    OBB obb = obbs[gl_InstanceIndex].obb;
    // if the signs are different, then subtract
    // abs add, then keep the sign
    vec3 obbVertex = vec3(0.0,0.0,0.0);
    obbVertex.x = sign(inPosition.x)*(abs(inPosition.x) + obb.half_extents.x);
    obbVertex.y = sign(inPosition.x)*(abs(inPosition.y) + obb.half_extents.y);
    obbVertex.z = sign(inPosition.x)*(abs(inPosition.z) + obb.half_extents.z);
    obbVertex += obb.center;
    mat3 rotationMatrix;
    rotationMatrix[0] = obb.directionU;
    rotationMatrix[1] = obb.directionV;
    rotationMatrix[2] = obb.directionW;
    gl_Position = projView * vec4(rotationMatrix * obbVertex, 1.0);
}
