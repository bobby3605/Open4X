#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in flat vec4 base_color_factor;
layout(location = 1) in vec2 frag_tex_coord;
layout(location = 2) in flat uint sampler_index;
layout(location = 3) in flat uint base_texture_index;
layout(location = 4) in flat uint normalIndex;
layout(location = 5) in flat uint metallic_roughness_index;
layout(location = 6) in flat uint ao_index;
layout(location = 7) in vec3 world_pos;
layout(location = 8) in vec3 normal;
layout(location = 9) in flat vec3 cam_pos;
layout(location = 10) in flat float normal_scale;
layout(location = 11) in flat float metallic_factor;
layout(location = 12) in flat float roughness_factor;
layout(location = 13) in flat float occlusion_strength;
layout(set = 2, binding = 0) uniform sampler samplers[];
layout(set = 2, binding = 1) uniform texture2D base_textures[];
layout(set = 2, binding = 2) uniform texture2D normal_textures[];
layout(set = 2, binding = 3) uniform texture2D metallic_roughness_textures[];
layout(set = 2, binding = 4) uniform texture2D ao_textures[];

layout(location = 0) out vec4 out_color;

vec3 light_pos = (0,0,0);
vec3 light_color = (1,1,1);

// https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/1.2.lighting_textured/1.2.pbr.fs
vec3 get_normal_from_map() {
    // fix for default normal map
    if (normal_index == 0) {
        return normal;
    } else {
        vec3 tangent_normal =
            (normal_scale *
             texture(sampler2D(normals[nonuniformEXT(normal_index)], samplers[nonuniformEXT(sampler_index)]), frag_tex_coord)
                 .rgb) *
                2.0 -
            1.0;

        vec3 Q1 = dFdx(world_pos);
        vec3 Q2 = dFdy(world_pos);
        vec2 st1 = dFdx(frag_tex_coord);
        vec2 st2 = dFdy(frag_tex_coord);

        vec3 N = normalize(normal);
        vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
        vec3 B = -normalize(cross(N, T));
        mat3 TBN = mat3(T, B, N);

        return normalize(TBN * tangent_normal);
    }
}

vec3 diffuse(){
    vec3 norm = get_normal_from_map();
    vec3 light_dir = normalize(light_pos - world_pos)
    float diff = max(dot(norm, light_dir), 0);
    return diff * light_color;
}

void main(){
    vec4 texture_color = texture(sampler2D(base_textures[nonuniformEXT(base_texture_index)], samplers[nonuniformEXT(sampler_index)]), frag_tex_coord);
    out_color = diffuse() * base_color_factor * texture_color;
}
