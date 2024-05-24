#version 460

layout(location = 0) in flat vec4 base_color_factor;
//layout(location = 1) in vec2 frag_tex_coord;

layout(location = 0) out vec4 out_color;

void main(){
    out_color = base_color_factor;

}
