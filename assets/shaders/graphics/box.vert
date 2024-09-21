#version 460

layout(binding = 0) uniform Globals {
    mat4 proj_view;
    vec3 cam_pos;
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
    uint instances_offset;
    uint instance_count;
};

layout(set = 1, binding = 0) readonly buffer ObjectCullingData { ObjectCullData objects[]; };
layout(set = 1, binding = 1) readonly buffer VisibilityBuffer { uint visibilityBuffer[]; };
layout(set = 1, binding = 2) readonly buffer ObjectInstanceIDs { uint objectInstanceIDs[]; };

layout(location = 0) in vec3 in_position;

layout(location = 0) out vec4 out_color;

void main() {
    ObjectCullData object = objects[gl_InstanceIndex];
    OBB obb = object.obb;
    mat3 rotation_matrix;
    rotation_matrix[0] = obb.directionU;
    rotation_matrix[1] = obb.directionV;
    rotation_matrix[2] = obb.directionW;
    // if the signs are different, then subtract
    // abs add, then keep the sign
    vec3 obb_vertex = in_position;
    // half extents is the scale, so it comes first
    obb_vertex.x = sign(obb_vertex.x)*(abs(obb_vertex.x) + obb.half_extents.x);
    obb_vertex.y = sign(obb_vertex.y)*(abs(obb_vertex.y) + obb.half_extents.y);
    obb_vertex.z = sign(obb_vertex.z)*(abs(obb_vertex.z) + obb.half_extents.z);
    // then rotate
    obb_vertex = rotation_matrix * obb_vertex;
    // then translate
    obb_vertex += obb.center;
    gl_Position = proj_view * vec4(obb_vertex, 1.0);
    if(visibilityBuffer[objectInstanceIDs[object.instances_offset]] == 1) {
        out_color = vec4(0.0,1.0,0.0,1.0);
    } else {
        out_color = vec4(1.0,0.0,0.0,1.0);
    }
}
