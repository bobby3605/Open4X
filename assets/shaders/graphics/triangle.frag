#version 460

layout(location = 0) in flat vec4 base_color_factor;
layout(location = 1) in vec2 frag_tex_coord;

layout(push_constant) uniform constants { vec4 push_constant_color; };

layout(location = 0) out vec4 out_color;

void main(){
    out_color = push_constant_color;
}
