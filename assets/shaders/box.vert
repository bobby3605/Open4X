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

layout(set = 1, binding = 0) readonly buffer ObjectCullingData { ObjectCullData objectData[]; };
layout(set = 1, binding = 1) readonly buffer VisibilityBuffer { bool visibilityBuffer[]; };

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec4 outColor;

void main() {
    ObjectCullData object = objectData[gl_InstanceIndex];
    OBB obb = object.obb;
    // if the signs are different, then subtract
    // abs add, then keep the sign
    vec3 obbVertex = vec3(0.0,0.0,0.0);
    obbVertex.x = sign(inPosition.x)*(abs(inPosition.x) + obb.half_extents.x);
    obbVertex.y = sign(inPosition.y)*(abs(inPosition.y) + obb.half_extents.y);
    obbVertex.z = sign(inPosition.z)*(abs(inPosition.z) + obb.half_extents.z);
    obbVertex += obb.center;
    mat3 rotationMatrix;
    rotationMatrix[0] = obb.directionU;
    rotationMatrix[1] = obb.directionV;
    rotationMatrix[2] = obb.directionW;
    gl_Position = projView * vec4(rotationMatrix * obbVertex, 1.0);
    if(visibilityBuffer[object.firstInstanceID]) {
        outColor = vec4(0.0,1.0,0.0,1.0);
    } else {
        outColor = vec4(1.0,0.0,0.0,1.0);
    }
}
