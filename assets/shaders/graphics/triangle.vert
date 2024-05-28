#version 460

layout(binding = 0) uniform Globals {
    mat4 proj_view;
    vec3 cam_pos;
};

struct InstanceData {
    mat4 model_matrix;
};

struct MaterialData {
    vec4 base_color_factor;
    uint sampler_index;
    uint image_index;
    uint normal_index;
    float normal_scale;
    uint metallic_roughness_index;
    float metallic_factor;
    float roughness_factor;
    uint ao_index;
    float occlusion_strength;
};

layout(std140, set = 1, binding = 0) readonly buffer Instances { InstanceData instances[]; };
layout(std140, set = 1, binding = 1) readonly buffer Materials { MaterialData materials[]; };
layout(set = 1, binding = 2) readonly buffer CulledInstanceIndices { uint culled_instance_indices[]; };
layout(set = 1, binding = 3) readonly buffer CulledMaterialIndices { uint culled_material_indices[]; };

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 tangent;

layout(location = 0) out vec4 base_color_factor;
layout(location = 1) out vec2 frag_texcoord;
layout(location = 2) out uint sampler_index;
layout(location = 3) out uint image_index;
layout(location = 4) out uint normal_index;
layout(location = 5) out uint metallic_roughness_index;
layout(location = 6) out uint ao_index;
layout(location = 7) out vec3 world_pos;
layout(location = 8) out vec3 normal_out;
layout(location = 9) out vec3 cam_pos_out;
layout(location = 10) out float normal_scale;
layout(location = 11) out float metallic_factor;
layout(location = 12) out float roughness_factor;
layout(location = 13) out float occulsion_strength;

void main() {
    /*
    MaterialData material = materials[culled_material_indices[gl_BaseInstance]];
    InstanceData instance = instances[culled_instance_indices[gl_InstanceIndex]];
    */
    MaterialData material = materials[0];
    InstanceData instance = instances[gl_InstanceIndex];

    vec4 vert_pos = instance.model_matrix * vec4(in_position, 1.0);
    gl_Position = proj_view * vert_pos;

    base_color_factor = material.base_color_factor;

    sampler_index = material.sampler_index;
    image_index = material.image_index;
    normal_index = material.normal_index;
    metallic_roughness_index = material.metallic_roughness_index;
    ao_index = material.ao_index;

    normal_scale = material.normal_scale;
    metallic_factor = material.metallic_factor;
    roughness_factor = material.roughness_factor;
    occulsion_strength = material.occlusion_strength;

    frag_texcoord = tex_coord;
    world_pos = vec3(vert_pos);
    normal_out = normal;
    cam_pos_out = cam_pos;
}
