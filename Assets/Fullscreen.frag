#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 uv_coords;

layout(location = 0) out vec4 frag_color;

layout(binding = 0) uniform sampler2D raytraced_image;

const float EXPOSURE = 1.0;

void main()
{
	const vec2 uv = { uv_coords.s, 1.0 - uv_coords.t };

	const vec3 hdr_color = texture(raytraced_image, uv).rgb;

	// const vec3 mapped = vec3(1.0) - exp(-hdr_color * EXPOSURE);

	frag_color = vec4(hdr_color, 1.0);
}
