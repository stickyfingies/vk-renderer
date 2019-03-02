#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 uv_coords;

layout(location = 0) out vec4 frag_color;

layout(binding = 0) uniform sampler2D raytraced_image;

void main()
{
	frag_color = texture(raytraced_image, vec2(uv_coords.s, 1.0 - uv_coords.t));
}
