#version 460

layout(location = 0) in flat vec4 base_color_factor;
layout(location = 1) in vec2 frag_tex_coord;
layout(location = 2) in flat uint sampler_index;
layout(location = 3) in flat uint image_index;
layout(set = 2, binding = 0) uniform sampler samplers[];
layout(set = 2, binding = 1) uniform texture2D base_textures[];

layout(location = 0) out vec4 out_color;

void main(){
    vec4 texture_color = texture(sampler2D(base_textures[nonuniformEXT(base_texture_index)], samplers[nonuniformEXT(sampler_index)]), frag_tex_coord)
    out_color = base_color_factor * texture_color;
}
